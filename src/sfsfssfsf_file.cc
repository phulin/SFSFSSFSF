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
	debug_print("in SFSFSSFSF_File(); setting location");
	location = _location;

	bool exists;
	ifstream foo(location.c_str());
	exists = (bool)foo;

	size_t total_bytes_read = 0;
	uint64_t file_bytes = 0;
	uint8_t *decode_ptr;
	FILE *pipein = NULL;
	if (exists) {
		char command[COMMAND_LEN];
		snprintf(command, COMMAND_LEN,
				 "ffmpeg -i %s -f u16le pipe:",
				 location.c_str());

		debug_print("in SFSFSSFSF_File(); about to open ffmpeg");
		pipein = popen(command, "r");
		if (!pipein) err("Couldn't open ffmpeg");
		debug_print("in SFSFSSFSF_File(); setting location");

		::decode_bits(pipein, (uint8_t *)&pfi, sizeof(struct pstat));
		if (pfi.magic != SFSFSSFSF_MAGIC) throw "Not an SFSFSSFSF";
	}
	else {
		pfi.magic = SFSFSSFSF_MAGIC
		pfi.pst_mode = S_IFREG | 0777;
		pfi.pst_size = 0;
		pfi.pst_atime = (uint64_t)time(NULL);
		pfi.pst_ctime = pfi.pst_atime;
		pfi.pst_mtime = pfi.pst_atime;
	}
	
	file_bytes = pfi.pst_size;
	// hopefully malloc will be okay with this
	data = new uint8_t[file_bytes];

	while (!feof(pipein) && (total_bytes_read < file_bytes)) {
		size_t bytes_read;

		bytes_read = ::decode_bits(pipein, decode_ptr, SFSFSSFSF_CHUNK);
		if (bytes_read == 0) { delete data; throw "Pipe error on read"; }

		total_bytes_read += bytes_read;
		decode_ptr += bytes_read;
	}
	// now data has been decoded and loaded into memory
	if (pipein) pclose(pipein);
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

// returns number of bytes encoded
// num_encode: bytes to encode
// precondition: num_bytes <= SFSFSSFSF_CHUNK
size_t encode_bits(FILE *pipein, FILE *pipeout, uint8_t *encode_buf, size_t num_bytes)
{
	int bit_index = 0;
	static uint16_t pcm_buf[8 * SFSFSSFSF_CHUNK];
	uint8_t *encode_ptr = encode_buf;

	while ((size_t)(encode_ptr - encode_buf) < num_bytes && !feof(pipein)) {
		unsigned int i;
		size_t num_written, num_read;

		size_t num_to_read = num_bytes - (encode_ptr - encode_buf);
		if (num_to_read >= SFSFSSFSF_CHUNK)
			num_to_read = SFSFSSFSF_CHUNK;
		num_read = fread(pcm_buf, 2, num_to_read, pipein);
		if (num_read == 0) break;

		for (i = 0; i < num_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
				encode_ptr++;
			}

			// only encode in things where it's (basically) invisible
			if (SFSFSSFSF_ENCODABLE(pcm_buf[i])) {
				// pick out and set a bit
				uint16_t mask = 1 << bit_index;
				pcm_buf[i] = (pcm_buf[i] & ~(0x1)) | ((*encode_ptr & mask) >> bit_index);
				bit_index++;
			}
		}
		num_written = fwrite(pcm_buf, 2, num_read, pipeout);
		if (num_written == 0) err("Pipe error on write");
	}

	return (size_t)(encode_ptr - encode_buf);
}

void SFSFSSFSF_File::fsync()
{
	FILE *pipeout, *pipein;
	char command[COMMAND_LEN], tmpname[COMMAND_LEN];

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -loglevel quiet -i %s -f u16le pipe:",
			 location.c_str());

	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	snprintf(tmpname, COMMAND_LEN,
			 "%s.tmp", location.c_str());

	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
		     "ffmpeg -y -loglevel quiet -f u16le -ac 2 -ar 44100 -i pipe: -f ipod -acodec alac %s", tmpname);

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2)");

	size_t num_encoded = encode_bits(pipein, pipeout, (uint8_t *)&pfi, sizeof pfi);
	if (num_encoded == 0) throw "Couldn't encode header";

	uint8_t *encode_ptr = data;
	while (num_encoded > 0 && encode_ptr - data < pfi.pst_size) {
		num_encoded = encode_bits(pipein, pipeout, encode_ptr, SFSFSSFSF_CHUNK);
		encode_ptr += num_encoded;
	}

	pclose(pipeout);

	rename(tmpname, location.c_str());
}
