// TODO: preserve metadata
// TODO: parallelize?
// TODO: insert minimum number of bits
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#include <config.h>

// number of 16-bit ints in a chunk
#define CHUNK 4096
// maximum command length
#define COMMAND_LEN 8192
// storage density, in bits per sample
#define STORAGE_DENSITY 1
// minimum number of bits in a sample before we encode a bit
// 

// endianness
#ifdef WORDS_BIGENDIAN
  #define FORMAT "u16be"
#else
  #define FORMAT "u16le"
#endif

#define err(msg) { perror(msg); \
				   exit(1); }

int main(int argc, char *argv[]) {
	FILE *encode_f = NULL, *pipein = NULL, *pipeout = NULL;
	char command[COMMAND_LEN], tmpname[COMMAND_LEN];
	uint16_t pcm_buf[CHUNK];
	uint8_t encode_buf[CHUNK], *encode_ptr = encode_buf + CHUNK;
	int bit_index = 0;
	bool done_encoding = false;

	if (argc < 3) err("Not enough arguments");

	encode_f = fopen(argv[2], "rb");
	if (!encode_f) err("Couldn't open file to encode");

	snprintf(tmpname, COMMAND_LEN,
			 "%s.tmp", argv[1]);

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -loglevel quiet -i %s -f " FORMAT " pipe:", argv[1]);
	// printf("%s\n", command);

	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
		     "ffmpeg -y -loglevel quiet -f " FORMAT " -ac 2 -ar 44100 -i pipe: -f ipod -acodec alac %s", tmpname);
	// printf("%s\n", command);

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2)");

	while (!feof(pipein)) {
		int i;
		size_t num_encode, num_read, num_written;

		num_read = fread(pcm_buf, 2, CHUNK, pipein);
		if (num_read == 0) err("Pipe error on read");

		// TODO: This needs optimization
		for (i = 0; i < num_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
				encode_ptr++;
			}
			if (encode_ptr - encode_buf >= CHUNK) {
				encode_ptr = encode_buf;
				num_encode = fread(encode_buf, 1, CHUNK, encode_f);
				if (num_encode == 0) {
					done_encoding = true;
				}
			}

			if (pcm_buf[i] & 0xFFF0) {
				if (done_encoding) {
					pcm_buf[i] &= 0xFFFE;
				}
				else {
					uint16_t mask = 1 << bit_index;
					pcm_buf[i] ^= (*encode_ptr & mask) >> bit_index;
				}
				bit_index++;
			}
		}

		num_written = fwrite(pcm_buf, 2, num_read, pipeout);
		if (num_written == 0) err("Pipe error on write");
	}

	pclose(pipeout);
	pclose(pipein);

	if (rename(tmpname, argv[1]) == -1) err("Couldn't move tempfile back to original location");

	return 0;
}
