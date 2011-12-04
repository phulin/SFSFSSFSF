// TODO: preserve metadata
// TODO: parallelize?
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
#define MIN_MASK 0xFFF0

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

// decode up to maxlen bytes into decode_buf
// pcm_buf should have enough space for 16 * maxlen bytes
size_t decode_bytes(FILE *pipein, uint8_t *decode_buf, size_t maxlen, uint16_t pcm_buf)
{
	uint16_t *pcm_buf;
	size_t samples_len = 8 * maxlen, num_read;
	int i;

	num_read = fread(pcm_buf, 2, samples_len, pipein);

	for (i = 0; i < numread; i++) {
		if (bit_index >= 8) {
			bit_index = 0;
			decode_buf++;
		}

		if (pcm_buf[i] & MIN_MASK) {
			*decode_buf |= (*decode_buf & 0x1) >> bit_index;
			bit_index++;
		}
	}

	if (num_read % 8 != 0) err("Non-byte-aligned read");
	return num_read / 8;
}

void decode(FILE *pipein, char *filename)
{
	int bit_index = 0;
	uint8_t decode_buf[CHUNK], *decode_ptr = decode_buf;
	uint16_t pcm_buf[16 * CHUNK];
	FILE *f;
	size_t num_read;
	struct stat fi;
	struct pstat pfi;
	uint64_t file_size, bytes_read;

	f = fopen(filename, "wb");
	if (!f) err("Couldn't open file to decode");

	num_read = decode_bytes(pipein, &pfi, sizeof(pfi), pcm_buf);
	if (num_read < sizeof(pfi)) err("Couldn't read header");

	while (!feof(pipein) && bytes_read < file_size) {
		size_t num_written;
		num_read = decode_bytes(pipein, decode_buf, CHUNK, pcm_buf);
		if (num_read == 0) err("Couldn't decode");

		bytes_read += num_read;
		num_written = fwrite(decode_buf, 1, num_read, f);
		if (num_written == 0) err("Pipe error on write");
	}
}

void encode(FILE *pipein, char *filename)
{
	int bit_index = 0;
	bool still_encoding = true;
	uint16_t pcm_buf[CHUNK];
	uint8_t encode_buf[CHUNK], *encode_ptr = encode_buf + CHUNK;
	FILE *f, *pipeout;
	size_t num_read;
	struct stat fi;
	struct pstat pfi;
	char command[COMMAND_LEN];

	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
		     "ffmpeg -y -loglevel quiet -f " FORMAT " -ac 2 -ar 44100 -i pipe: -f ipod -acodec alac %s", tmpname);

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2)");

	f = fopen(filename, "rb");
	if (!f) err("Couldn't open file to encode");

	// We'll include a header at the beginning of an encoded file to indicate length.
	if (stat(filename, &fi) != 0) err("Couldn't stat file.");
	if (fi.st_mode & S_IFMT != S_IFREG) err("Not a regular file.");
	pstat_write(&fi, &pfi);
	memcpy(pcm_buf, &pfi, sizeof(pfi));
	num_read = sizeof(pfi);

	while (!feof(pipein)) {
		int i;
		size_t num_encode, num_written;

		for (i = 0; i < num_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
				encode_ptr++;
			}
			if (encode_ptr - encode_buf >= CHUNK) {
				encode_ptr = encode_buf;
				num_encode = fread(encode_buf, 1, CHUNK, f);
				if (num_encode == 0) {
					still_encoding = false;
				}
			}

			if (still_encoding && (pcm_buf[i] & MIN_MASK)) {
				uint16_t mask = 1 << bit_index;
				pcm_buf[i] ^= (*encode_ptr & mask) >> bit_index;
				bit_index++;
			}
		}

		num_written = fwrite(pcm_buf, 2, num_read, pipeout);
		if (num_written == 0) err("Pipe error on write");
	}

	num_read = fread(pcm_buf, 2, CHUNK, pipein);
	if (num_read == 0) err("Pipe error on read");
}

int main(int argc, char *argv[])
{
	FILE *encode_f = NULL, *pipein = NULL;
	char command[COMMAND_LEN], tmpname[COMMAND_LEN];
	struct stat fileinfo;
	sfs_mode mode;

	if (argc < 4) usage(argv[0]);
	if (strcmp(argv[1], "-e") == 0)
		mode = SFS_ENCODE;
	else if (strcmp(argv[1], "-d") == 0)
		mode = SFS_DECODE;
	else
		usage(argv[0]);

	snprintf(tmpname, COMMAND_LEN,
			 "%s.tmp", argv[2]);

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -loglevel quiet -i %s -f " FORMAT " pipe:", argv[2]);

	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	if (mode == SFS_ENCODE)
		encode(pipein, argv[3]);
	else if (mode == SFS_DECODE)
		decode(pipein, argv[3]);

	pclose(pipeout);
	pclose(pipein);

	if (rename(tmpname, argv[1]) == -1) err("Couldn't move tempfile back to original location");

	return 0;
}
