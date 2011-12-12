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

#define err(msg) { perror(msg); exit(1); }

// decode up to maxbytes bytes into decode_buf
// precondition: maxbytes <= SFSFSSFSF_CHUNK
// returns number of bytes decoded.
// will decode maxbytes bytes unless at EOF
size_t SFSFSSFSF_File::decode_bits(uint8_t *decode_ptr, size_t maxbytes)
{
	size_t samples_read = 0, bits_read = 0;
	size_t maxbits = 8 * maxbytes;
	static uint16_t pcm_buf[8 * SFSFSSFSF_CHUNK];
	int bit_index = 0;

	*decode_ptr = 0;
	
	while (bits_read < maxbits) {
		samples_read = fread(pcm_buf, 2, maxbits - bits_read, pipein);
		if (samples_read == 0) break;

		for (int i = 0; i < samples_read; i++) {
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

	if (bits_read % 8 != 0) err("Non-byte-aligned read");
	return bits_read / 8;
}

// TODO: parallelize - important!
// decode file, etc.
SFSFSSFSF_File::SFSFSSFSF_File(char *filename, char *mode)
{
	size_t total_bytes_read = 0;
	uint64_t file_bytes = 0;

	struct pstat pst;
	decode_bits((uint8_t *)&pst, sizeof(struct pstat));
	if (pst.magic != SFSFSSFSF_MAGIC) err("Not an SFSFSSFSF");
	file_bytes = pst.pst_size;

	// initialize data; hopefully malloc will be okay with this
	data = new uint8_t[file_bytes];
	if (!data) err("Couldn't allocate memory");
	decode_ptr = data;

	while (!feof(pipein) && (total_bytes_read < file_bytes)) {
		size_t bytes_read;

		bytes_read = decode_bytes(pipein, decode_ptr, SFSFSSFSF_CHUNK);
		if (bytes_read == 0) err("Pipe error on read");

		total_bytes_read += bytes_read;
		decode_ptr += bytes_read;
	}
	// now data has been decoded and loaded into memory
}

size_t SFSFSSFSF_FILE::read(size_t num_bytes, uint8_t *buf)
{
	return 0;
}

size_t SFSFSSFSF_FILE::write(size_t num_bytes, uint8_t *buf)
{
	return 0;
}

static int sfsfssfsf_getattr(const char *path, struct stat *stbuf)
{
	return -1;
}

static int sfsfssfsf_open(const char *path, struct fuse_file_info *fi)
{
	return -1;
}

static int sfsfssfsf_read(const char *path, char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	return -1;
}

static int sfsfssfsf_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	return -1;
}

int main(int argc, char *argv[])
{
	struct fuse_operations sfsfssfsf_ops;
	sfsfssfsf_ops.getattr = sfsfssfsf_getattr;
	sfsfssfsf_ops.open = sfsfssfsf_open;
	sfsfssfsf_ops.read = sfsfssfsf_read;
	sfsfssfsf_ops.readdir = sfsfssfsf_readdir;

	return fuse_main(argc, argv, &sfsfssfsf_ops, NULL);
}