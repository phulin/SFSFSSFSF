#include <sfsfssfsf.h>

// decode up to maxbytes bytes into decode_buf
// precondition: maxbytes <= SFSFSSFSF_CHUNK
// returns number of bytes decoded.
// will decode maxbytes bytes unless at EOF
size_t decode_bits(FILE *pipein, uint8_t *decode_ptr, size_t maxbytes)
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
SFSFSSFSF_File::SFSFSSFSF_File(string _location, string mode)
{
	location = _location;

	char command[COMMAND_LEN];
	snprintf(command, COMMAND_LEN,
			 "ffmpeg -i %s -f " SFSFSSFSF_FORMAT " pipe:",
			 location.c_str());

	FILE *pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	size_t total_bytes_read = 0;
	uint64_t file_bytes = 0;

	::decode_bits(pipein, (uint8_t *)&pfi, sizeof(struct pstat));
	if (pfi.magic != SFSFSSFSF_MAGIC) throw "Not an SFSFSSFSF";
	file_bytes = pfi.pst_size;

	// initialize data; hopefully malloc will be okay with this
	data = new uint8_t[file_bytes];
	uint8_t *decode_ptr = data;

	while (!feof(pipein) && (total_bytes_read < file_bytes)) {
		size_t bytes_read;

		bytes_read = ::decode_bits(pipein, decode_ptr, SFSFSSFSF_CHUNK);
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
