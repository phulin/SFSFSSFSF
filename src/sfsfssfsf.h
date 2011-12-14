#ifndef HAVE_SFSFSSFSF_H
#define HAVE_SFSFSSFSF_H

using namespace std;
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <ios>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include <pstat.h>

// number of 16-bit ints in a chunk
// note: this MUST be larger than sizeof(pstat)
#define SFSFSSFSF_CHUNK 16384
// maximum command length
#define COMMAND_LEN 8192
// minimum number of bits in a sample before we encode a bit
// MUST guarantee that SFSFSSFSF_ENCODABLE(byte) is invariant on flipping
// last bit of byte.
// TODO: this really _should_ be hidden in the FS options
// to allow user-defined, runtime tradeoff between security
// and space
#define SFSFSSFSF_ENCODABLE(byte) ((0x7FF0 > byte) || (byte > 0x800F))

#define DEBUG

#ifdef DEBUG
#define debug_print(fmt) printf(fmt)
#else
#define debug_print(fmt) do {} while (0)
#endif

using namespace std;

class SFSFSSFSF_File
{
	string location; // in what file is it stored?
	uint8_t *data;
	uint8_t *cur_ptr;
	size_t total_bits_read;
	struct pstat pfi;
	bool is_superblock;

public:
	SFSFSSFSF_File(string, string);
	~SFSFSSFSF_File();
	size_t read(off_t, size_t, uint8_t *);
	size_t write(off_t, size_t, uint8_t *);
	void fsync();

private:
	inline size_t bound_num_bytes(off_t, size_t);
};

namespace SFSFSSFSF {
	size_t decode_bits(FILE *, uint8_t *, size_t);
	size_t encode_bits(FILE *, FILE *, uint8_t *, size_t);
	FILE *pipein_from(string);
	FILE *pipeout_to(string);

	void superblock_write(string location);
	void superblock_read(string location);
}
using namespace SFSFSSFSF;


// this is a hack but whatever
static void err(const char *msg)
{
	char *error_s = strerror(errno);
	char *ptr = new char[strlen(msg) + strlen(error_s) + 8];
	sprintf(ptr, "%s: %s", msg, error_s);
}

static int print_err(char *e)
{
	fprintf(stderr, "%s\n", e);
	delete e;
	return -1;
}

extern string audiofile_list_file;
extern string superblock_file;

#endif
