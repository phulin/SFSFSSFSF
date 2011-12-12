#ifndef HAVE_SFSFSSFSF_H
#define HAVE_SFSFSSFSF_H

using namespace std;
#include <cerrno>
#include <cstdio>

#include <stdint.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <pstat.h>

class SFSFSSFSF_File
{
	FILE *pipein;
	uint8_t *data;
	size_t total_bits_read;
	size_t cur_pos;
	struct pstat pfi;

public:
	SFSFSSFSF_File::SFSFSSFSF_File(char *filename, char *mode);
	size_t read(size_t num_bytes, uint8_t *buf);
	size_t write(size_t num_bytes, uint8_t *buf);

private:
	size_t decode_bits(uint8_t *decode_ptr, size_t maxbits);
};

#endif
