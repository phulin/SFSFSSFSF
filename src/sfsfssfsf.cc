// sfsfssfsf.cc: Implementation of main filesystem
// TODO: preserve metadata
// TODO: check low-order bits for entropy
// TODO: sliding-window entropy estimator
// TODO: be more flexible with atypical ALAC numchannels/etc

#include <sfsfssfsf.h>

// number of 16-bit ints in a chunk
// note: this MUST be larger than sizeof(pstat)
#define SFSFSSFSF_CHUNK 4096
// maximum command length
#define COMMAND_LEN 8192
// minimum number of bits in a sample before we encode a bit
// MUST guarantee that SFSFSSFSF_ENCODABLE(byte) is invariant on flipping
// last bit of byte.
// TODO: this really _should_ be hidden in the FS options
// to allow user-defined, runtime tradeoff between security
// and space
#define SFSFSSFSF_ENCODABLE(byte) ((0x7FF0 > byte) || (byte > 0x800F))

// endianness
#ifdef WORDS_BIGENDIAN
  #define SFSFSSFSF_FORMAT "u16be"
#else
  #define SFSFSSFSF_FORMAT "u16le"
#endif

static void err(const char *msg)
{
	char *error_s = strerror(errno);
	char *ptr = new char[strlen(msg) + strlen(error_s) + 8];
	sprintf(ptr, "%s: %s", msg, error_s);
}

static void print_err(char *e)
{
	fprintf(stderr, "%s\n", e);
	delete e;
}

static char *superblock_file;

// decode up to maxbytes bytes into decode_buf
// precondition: maxbytes <= SFSFSSFSF_CHUNK
// returns number of bytes decoded.
// will decode maxbytes bytes unless at EOF
size_t SFSFSSFSF_File::decode_bits(FILE *pipein, uint8_t *decode_ptr, size_t maxbytes)
{
	size_t samples_read = 0, bits_read = 0;
	size_t maxbits = 8 * maxbytes;
	static uint16_t pcm_buf[8 * SFSFSSFSF_CHUNK];
	int bit_index = 0;

	*decode_ptr = 0;
	
	while (bits_read < maxbits) {
		samples_read = fread(pcm_buf, 2, maxbits - bits_read, pipein);
		if (samples_read == 0) break;

		for (unsigned int i = 0; i < samples_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
				decode_ptr++;
				*decode_ptr = 0;
			}

			if (SFSFSSFSF_ENCODABLE(pcm_buf[i])) {
				// extract bit
				*decode_ptr |= (pcm_buf[i] & 0x1) << bit_index;
				bit_index++;
				bits_read++;
			}
		}
	}

	if (bits_read % 8 != 0) throw "Non-byte-aligned read";
	return bits_read / 8;
}

// TODO: parallelize - important!
// TODO: support mode
// TODO: locking mechanism for files
// decode file, etc.
SFSFSSFSF_File::SFSFSSFSF_File(char *_location, char *mode)
{
	location = _location;

	char command[COMMAND_LEN];
	snprintf(command, COMMAND_LEN,
			 "ffmpeg -i %s -f " SFSFSSFSF_FORMAT " pipe:",
			 location);

	FILE *pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	size_t total_bytes_read = 0;
	uint64_t file_bytes = 0;

	decode_bits(pipein, (uint8_t *)&pfi, sizeof(struct pstat));
	if (pfi.magic != SFSFSSFSF_MAGIC) throw "Not an SFSFSSFSF";
	file_bytes = pfi.pst_size;

	// initialize data; hopefully malloc will be okay with this
	data = new uint8_t[file_bytes];
	uint8_t *decode_ptr = data;

	while (!feof(pipein) && (total_bytes_read < file_bytes)) {
		size_t bytes_read;

		bytes_read = decode_bits(pipein, decode_ptr, SFSFSSFSF_CHUNK);
		if (bytes_read == 0) { delete data; throw "Pipe error on read"; }

		total_bytes_read += bytes_read;
		decode_ptr += bytes_read;
	}
	// now data has been decoded and loaded into memory
	pclose(pipein);

	cur_ptr = data;
}

SFSFSSFSF_File::~SFSFSSFSF_File()
{
	delete data;
}

// make sure we don't read or write past end of data buf
inline size_t SFSFSSFSF_File::bound_num_bytes(off_t offset, size_t num_bytes)
{
	size_t bytes_left = pfi.pst_size - offset;
	if (num_bytes > bytes_left)
		return bytes_left;
	else
		return num_bytes;
}

size_t SFSFSSFSF_File::read(off_t offset, size_t num_bytes, uint8_t *buf)
{
	num_bytes = bound_num_bytes(offset, num_bytes);

	memcpy(buf, data + offset, num_bytes);

	return num_bytes;
}

size_t SFSFSSFSF_File::write(off_t offset, size_t num_bytes, uint8_t *buf)
{
	num_bytes = bound_num_bytes(offset, num_bytes);

	memcpy(data + offset, buf, num_bytes);

	return num_bytes;
}

static int sfsfssfsf_open(const char *path, struct fuse_file_info *fi)
{
	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(superblock_file, NULL);
		fi->fh = (uint64_t)f;
	}
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

static int sfsfssfsf_getattr(const char *path, struct stat *stbuf)
{
	return -1;
}

static int sfsfssfsf_read(const char *path, char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	try {
		f->read(offset, size, (uint8_t *)buf);
	}
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

static int sfsfssfsf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	return filler(buf, "file", NULL, 0);
}

int main(int argc, char *argv[])
{
	struct fuse_operations ops;
	ops.getattr = sfsfssfsf_getattr;
	ops.open = sfsfssfsf_open;
	ops.read = sfsfssfsf_read;
	ops.readdir = sfsfssfsf_readdir;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s location fs-args...", argv[0]);
		return 1;
	}

	superblock_file = argv[1];
	char **argv_new = new char *[argc - 1];
	argv_new[0] = argv[0];
	memcpy(argv_new + 1, argv + 2, argc - 2);

	int res = fuse_main(argc, argv_new, &ops, NULL);

	delete argv_new;
	return res;
}
