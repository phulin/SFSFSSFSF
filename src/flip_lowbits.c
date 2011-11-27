#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// number of 32-bit ints in a chunk
#define CHUNK 4096
#define COMMAND_LEN 8192

#define err(msg) { fprintf(stderr, msg); exit(1); }

// TODO: preserve metadata
// TODO: parallelize?
// TODO: insert minimum number of bits

int main(int argc, char *argv[]) {
	FILE *fin, *pipein, *pipeout, *fout;
	char command[COMMAND_LEN];
	uint32_t buf[CHUNK];

	if (argc < 1) err("Not enough arguments.\n");

	fin = fopen(argv[1], "rb");
	if (!fin) err("Couldn't open file for reading.\n");

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -i %s -f u32be pipe:", argv[1]);
	
	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg.\n");

	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
		     "ffmpeg -f u32be -ac 2 -ar 44100 -i pipe: -acodec alac .tmp.%s", argv[1]);

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2).\n");

	while (!feof(pipein)) {
		int i;
		size_t numread;

		numread = fread(buf, 4, CHUNK, pipein);
		if (numread < 0) err("Pipe error on read.\n");

		for (i = 0; i < numread; i++) {
			buf[i] &= 0xFFFFFFFE;
		}
	}

	fclose(fin);
	pclose(pipein);
	pclose(pipeout);
	fclose(fout);
	return 0;
}
