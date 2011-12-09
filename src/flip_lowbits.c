// TODO: preserve metadata
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator
// TODO: actually make into filesystem

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <config.h>
#include <pstat.h>

// number of 16-bit ints in a chunk
// note: this MUST be larger than sizeof(pstat)
#define CHUNK 4096
// maximum command length
#define COMMAND_LEN 8192
// storage density, in bits per sample
// TODO: implement
#define STORAGE_DENSITY 1
// minimum number of bits in a sample before we encode a bit
// MUST guarantee that encodable(byte) is invariant on flipping
// last bit of byte.
#define encodable(byte) ((0x7FF0 > byte) || (byte > 0x800F))

// endianness
#ifdef WORDS_BIGENDIAN
  #define FORMAT "u16be"
#else
  #define FORMAT "u16le"
#endif

#define err(msg) { perror(msg); \
				   exit(1); }

typedef enum sfs_mode { SFS_ENCODE, SFS_DECODE } sfs_mode;

void usage(char *argv0)
{
	fprintf(stderr, "Usage: %s (-e|-d) audio-file (file-to-encode|destination)\n", argv0);
	exit(1);
}

// decode up to maxbits bits into decode_buf
// precondition: maxbits <= 8 * CHUNK
// bit_index is constant across calls!
// this means you can only decode one stream with this function.
// returns number of _BITS_ decoded.
static size_t decode_bits(FILE *pipein, uint8_t *decode_buf, size_t maxbits)
{
	size_t num_read, bits_read = 0;
	uint8_t *decode_ptr = decode_buf;
	int i;
	static uint16_t pcm_buf[8 * CHUNK];
	static int bit_index = 0;
	static bool first_time = true;

	num_read = fread(pcm_buf, 2, maxbits, pipein);
	if (num_read == 0) err("Pipe error on read");

	// make sure we zero the first time
	if (first_time) {
		*decode_ptr = 0;
		first_time = false;
	}

	for (i = 0; i < num_read; i++) {
		if (bit_index >= 8) {
			bit_index = 0;
			decode_ptr++;
			*decode_ptr = 0;
		}

		if (encodable(pcm_buf[i])) {
			// extract bit
			*decode_ptr |= (pcm_buf[i] & 0x1) << bit_index;
			bit_index++;
			bits_read++;
		}
	}

	return bits_read;
}

static void decode(FILE *pipein, char *filename)
{
	uint8_t decode_buf[CHUNK + sizeof(struct pstat)], *decode_ptr = decode_buf;
	FILE *f;
	size_t total_bits_read = 0, total_bits_written = 0;
	size_t pstat_bits = 8 * sizeof(struct pstat);
	size_t bits_read;
	struct pstat *pfi = (struct pstat *)decode_buf;
	uint64_t file_bits = 0;

	f = fopen(filename, "wb");
	if (!f) err("Couldn't open file to decode");

	// we decode the header in the beginning of the buffer
	// because we can't guarantee decoding an exact number of bytes
	// so overflow ends up as part of what we're actually decoding
	// the issue is that it's hard to avoid 'spill' in ways that
	// aren't super, super slow
	// note, bit index is stored in decode_bits, so we can throw out
	// non-byte-aligned data
	while (total_bits_read < pstat_bits) {
		total_bits_read += decode_bits(pipein, decode_ptr + (total_bits_read / 8), pstat_bits);
	}
	file_bits = 8 * pfi->pst_size;

	// now we start decoding for real
	decode_ptr += sizeof(struct pstat);

	bits_read = total_bits_read - pstat_bits;
	total_bits_read = 0;

	while (!feof(pipein) && (total_bits_read < file_bits)) {
		size_t bytes_written, bytes_to_write;
		size_t bits_to_write, bits_left;

		total_bits_read += bits_read;
		// this is how many bits are fully decoded and
		// ready to be written
		bits_to_write = total_bits_read - total_bits_written;
		bits_left = file_bits - total_bits_written;
		if (bits_to_write > bits_left)
			bits_to_write = bits_left;
		bytes_to_write = bits_to_write / 8;

		bytes_written = fwrite(decode_ptr, 1, bytes_to_write, f);
		if (bytes_written == 0) err("Pipe error on write");
		total_bits_written += 8 * bytes_written;

		// move partially-decoded byte to right place.
		decode_ptr[0] = decode_ptr[bytes_written];
		// and decode more bits
		bits_read = decode_bits(pipein, decode_ptr, 8 * CHUNK);
	}
}

static void encode(FILE *pipein, char *location, char *filename)
{
	int bit_index = 0;
	bool still_encoding = true;
	uint16_t pcm_buf[CHUNK];
	uint8_t encode_buf[CHUNK], *encode_ptr = encode_buf;
	FILE *f, *pipeout, *debug_f; // debug
	size_t num_read, num_encode;
	struct stat fi;
	struct pstat pfi;
	char command[COMMAND_LEN], tmpname[COMMAND_LEN];

	snprintf(tmpname, COMMAND_LEN,
			 "%s.tmp", location);

	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
		     "ffmpeg -y -loglevel quiet -f " FORMAT " -ac 2 -ar 44100 -i pipe: -f ipod -acodec alac %s", tmpname);

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2)");

	f = fopen(filename, "rb");
	if (!f) err("Couldn't open file to encode");
	debug_f = fopen("/tmp/pcm.out", "wb"); // debug

	// We'll include a header at the beginning of an encoded file to indicate length.
	if (stat(filename, &fi) != 0) err("Couldn't stat file.");
	if ((fi.st_mode & S_IFMT) != S_IFREG) err("Not a regular file.");
	pstat_write(&fi, &pfi);
	memcpy(encode_buf, &pfi, sizeof pfi);
	num_encode = sizeof pfi;

	while (!feof(pipein)) {
		int i;
		size_t num_written;

		num_read = fread(pcm_buf, 2, CHUNK, pipein);
		if (num_read == 0) err("Pipe error on read");

		for (i = 0; i < num_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
				encode_ptr++;
			}
			// need more data to encode
			if (encode_ptr - encode_buf >= num_encode) {
				encode_ptr = encode_buf;
				num_encode = fread(encode_buf, 1, CHUNK, f);
				if (num_encode == 0) {
					still_encoding = false;
				}
			}

			// only encode in things where it's (basically) invisible
			if (still_encoding && encodable(pcm_buf[i])) {
				// pick out and set a bit
				uint16_t mask = 1 << bit_index;
				pcm_buf[i] = (pcm_buf[i] & ~(0x1)) | ((*encode_ptr & mask) >> bit_index);
				bit_index++;
			}
		}

		num_written = fwrite(pcm_buf, 2, num_read, pipeout);
		if (num_written == 0) err("Pipe error on write");
		fwrite(pcm_buf, 2, num_read, debug_f); // debug
	}

	pclose(pipeout);

	if (rename(tmpname, location) == -1) err("Couldn't move tempfile back to original location");
}

int main(int argc, char *argv[])
{
	FILE *pipein = NULL;
	char command[COMMAND_LEN];
	sfs_mode mode = -1;

	if (argc < 4) usage(argv[0]);
	if (strcmp(argv[1], "-e") == 0)
		mode = SFS_ENCODE;
	else if (strcmp(argv[1], "-d") == 0)
		mode = SFS_DECODE;
	else
		usage(argv[0]);

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -loglevel quiet -i %s -f " FORMAT " pipe:", argv[2]);

	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	if (mode == SFS_ENCODE)
		encode(pipein, argv[2], argv[3]);
	else if (mode == SFS_DECODE)
		decode(pipein, argv[3]);

	pclose(pipein);

	return 0;
}
