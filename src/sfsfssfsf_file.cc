#include <sfsfssfsf.h>

// decode up to maxbytes bytes into decode_buf
// precondition: maxbytes <= SFSFSSFSF_CHUNK
// returns number of bytes decoded.
// will decode maxbytes bytes unless at EOF
size_t SFSFSSFSF::decode_bits(FILE *pipein, uint8_t *decode_ptr, size_t maxbytes)
{
	size_t samples_read = 0, bits_read = 0;
	size_t maxbits = 8 * maxbytes;
	static uint16_t pcm_buf[8 * SFSFSSFSF_CHUNK] = {0};
	int bit_index = 0;

	*decode_ptr = 0;
	
	while (bits_read < maxbits) {
		samples_read = fread(pcm_buf, 2, maxbits - bits_read, pipein);
		if (samples_read == 0) break;

		for (unsigned int i = 0; i < samples_read; i++) {
			if (bit_index >= 8) {
				bit_index = 0;
#ifdef VDEBUG
				cout<<(char)*decode_ptr;
#endif
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
	debug_print("\n");

	if (bits_read % 8 != 0) throw "Non-byte-aligned read";
	return bits_read / 8;
}

// TODO: parallelize - important!
// TODO: support mode
// TODO: locking mechanism for files
// decode file, etc.
SFSFSSFSF_File::SFSFSSFSF_File(string _location, string mode, bool force_overwrite)
{
#ifdef DEBUG
	cout<<"in SFSFSSFSF_File(); setting location: |"<<_location<<"|\n"<<flush;
#endif
	ref_count = 0;

	total_bits_read = 0;

	location = _location;

	uint8_t *decode_ptr;
	FILE *pipein = NULL;
	
	char command[COMMAND_LEN] = {0};
	snprintf(command, COMMAND_LEN,
			 "ffmpeg -i \"%s\" -f u16le pipe:",
			 location.c_str());

	debug_print("in SFSFSSFSF_File(); about to open ffmpeg\n");
	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg\n");

	debug_print("Opened ffmpeg okay\n");

	// hopefully malloc will be okay with this
	// FIXME: WTF?
	// zero data array
	data = new uint8_t[4194304];
	memset(data, 0, 4194304);

	debug_print("SFSFSSSF_File 1\n");

	decode_ptr =  (uint8_t *)&pfi;
	// if force_overwrite, create a new pstat struct
	// otherwise read in the one stored in the file
	if (force_overwrite) {
		debug_print("SFSFSSSF_File 2\n");

		debug_print("New file, using force_overwrite\n");
		pfi.magic = SFSFSSFSF_MAGIC;
		pfi.pst_mode = S_IFREG | 0777;
		pfi.pst_size = 0;
		pfi.pst_atime = (uint64_t)time(NULL);
		pfi.pst_ctime = pfi.pst_atime;
		pfi.pst_mtime = pfi.pst_atime;
	}
	else {
		debug_print("SFSFSSSF_File 3\n");
		decode_bits(pipein, decode_ptr, sizeof(struct pstat));
	}
	debug_print("SFSFSSSF_File 4\n");

	if (pfi.magic != SFSFSSFSF_MAGIC) throw "Not an SFSFSSFSF";
	
	uint64_t file_bytes = pfi.pst_size;
	debug_print("SFSFSSSF_File 5\n");

	// read in the rest of the file
	// if new file, won't enter while loop as file_bytes = 0
	size_t total_bytes_read = 0;
	decode_ptr = data;
	while (!feof(pipein) && (total_bytes_read < file_bytes)) {
		size_t bytes_read;

		bytes_read = decode_bits(pipein, decode_ptr, SFSFSSFSF_CHUNK);
		if (bytes_read == 0) { delete data; throw "Pipe error on read"; }

		total_bytes_read += bytes_read;
		decode_ptr += bytes_read;
	}
	debug_print("SFSFSSSF_File 6\n");

	// now data has been decoded and loaded into memory
	if (pipein) pclose(pipein);
	debug_print("SFSFSSSF_File 7\n");

}

SFSFSSFSF_File::~SFSFSSFSF_File()
{
	delete data;
}

// make sure we don't read or write past end of data buf
inline size_t SFSFSSFSF_File::bound_num_bytes(off_t offset, size_t num_bytes)
{
	size_t bytes_left = pfi.pst_size - offset;
	assert((size_t)offset <= pfi.pst_size);
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
	if (offset + num_bytes > pfi.pst_size) {
		pfi.pst_size = offset + num_bytes;
	}
	
	memcpy(data + offset, buf, num_bytes);

	return num_bytes;
}

// takes enough samples from pipein to get num_bytes bytes of data
// encodes data from encode_buf into samples from pipein
// and send the altered samples to pipeout
// unless still_encoding is false, in which case the data is passed unaltered.
// returns number of bytes encoded
// num_encode: bytes to encode
// precondition: num_bytes <= SFSFSSFSF_CHUNK
off_t SFSFSSFSF::encode_bits(FILE *pipein, FILE *pipeout, uint8_t *encode_buf, size_t num_bytes, bool still_encoding)
{
	int bit_index = 0;
	static uint16_t pcm_buf[8 * SFSFSSFSF_CHUNK] = {0};
	uint8_t *encode_ptr = encode_buf;

	assert(num_bytes <= SFSFSSFSF_CHUNK);
	while ((size_t)(encode_ptr - encode_buf) < num_bytes && !feof(pipein)) {
		unsigned int i;
		size_t num_written, num_read;

		size_t num_to_read = num_bytes - (encode_ptr - encode_buf);
		assert (num_to_read <= SFSFSSFSF_CHUNK);
		num_read = fread(pcm_buf, 2, num_to_read, pipein);
		if (num_read == 0) break;

		for (i = 0; i < num_read; i++) {
			// only encode in things where it's (basically) invisible
			if (SFSFSSFSF_ENCODABLE(pcm_buf[i]) && still_encoding) {
				// pick out and set a bit
				uint16_t mask = 1 << bit_index;
				// zero last bit and put the new thing in
				pcm_buf[i] = (pcm_buf[i] & ~(0x1)) | ((*encode_ptr & mask) >> bit_index);
				bit_index++;
			}

			if (bit_index >= 8) {
				bit_index = 0;
#ifdef VDEBUG
				cout<<(char)*encode_ptr;
#endif
				encode_ptr++;
			}
		}
		num_written = fwrite(pcm_buf, 2, num_read, pipeout);
		if (num_written == 0 && num_read > 0) err("Pipe error on write");
	}

	if(feof(pipein)) 
		return -1;

	assert(feof(pipein) || bit_index == 0);

	return (size_t)(encode_ptr - encode_buf);
}

void SFSFSSFSF_File::fsync()
{
	debug_print("entering fsync\n");
       
#ifdef DEBUG
	cout<<"fsyncing "<<location<<endl;
#endif
	FILE *pipeout, *pipein;
	char command[COMMAND_LEN], tmpname[COMMAND_LEN] = {0};

	snprintf(command, COMMAND_LEN, 
	         "ffmpeg -loglevel quiet -i \"%s\" -f u16le pipe:",
	         location.c_str());

	pipein = popen(command, "r");
	if (!pipein) err("Couldn't open ffmpeg");

	snprintf(tmpname, COMMAND_LEN,
	         "%s.tmp", location.c_str());

	debug_print("fsync 1\n");
	// we'll write to a separate file to avoid screwing things up
	snprintf(command, COMMAND_LEN,
	         "ffmpeg -y -loglevel quiet -f u16le -ac 2 -ar 44100 -i "\
	         "pipe: -f ipod -acodec alac \"%s\"", tmpname);
	debug_print("fsync 2\n");

	pipeout = popen(command, "w");
	if (!pipeout) err("Couldn't open ffmpeg (2)");
	debug_print("fsync 3\n");
	uint8_t * encode_ptr = (uint8_t *) &pfi;

	size_t num_encoded = encode_bits(pipein, pipeout, encode_ptr, sizeof(pfi) );
	if (num_encoded == 0) throw "Couldn't encode header";
	debug_print("fsync 4\n");

	encode_ptr = data;
	while (num_encoded >= 0) {
		
		num_encoded = encode_bits(pipein, pipeout, encode_ptr, SFSFSSFSF_CHUNK, 
	                          (size_t)(encode_ptr - data) < pfi.pst_size);
		encode_ptr += num_encoded;
	}
	debug_print("fsync 6\n");

	pclose(pipein);
	pclose(pipeout);

	rename(tmpname, location.c_str());
}

// self-explanatory helper functions
// don't forget to pclose them.
FILE *SFSFSSFSF::pipein_from(string location)
{
	ostringstream command;
	command << "ffmpeg -i \"" << location << "\" -f u16le pipe:";
	FILE *pipein = popen(command.str().c_str(), "r");
	if (!pipein) err("Could not open ffmpeg");

	return pipein;
}

FILE *SFSFSSFSF::pipeout_to(string location)
{
	ostringstream command;
	command << "ffmpeg -y -loglevel quiet -f u16le -ac 2 -ar 44100 -i pipe: -f" \
	           " ipod -acodec alac \"" << location << "\"";
	FILE *pipeout = popen(command.str().c_str(), "w");
	if (!pipeout) err("Could not open ffmpeg");

	return pipeout;
}

size_t SFSFSSFSF_File::get_size()
{
	return pfi.pst_size;
}

// returns true if out of references
bool SFSFSSFSF_File::release()
{
	ref_count--;
	if (ref_count == 0)
		return true;
	else
		return false;
}
