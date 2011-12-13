#ifndef HAVE_SFSFSSFSF_H
#define HAVE_SFSFSSFSF_H

using namespace std;
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdint.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <pstat.h>

class SFSFSSFSF_File
{
	char *location; // in what file is it stored?
	uint8_t *data;
	uint8_t *cur_ptr;
	size_t total_bits_read;
	struct pstat pfi;

public:
	SFSFSSFSF_File(char *, char *);
	~SFSFSSFSF_File();
	size_t read(off_t, size_t, uint8_t *);
	size_t write(off_t, size_t, uint8_t *);

private:
	size_t decode_bits(FILE *, uint8_t *, size_t);
	inline size_t bound_num_bytes(off_t, size_t);
};

#endif
