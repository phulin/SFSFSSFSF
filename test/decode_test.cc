#include <sfsfssfsf.h>

int main(int argc, char *argv[])
{
	FILE *pipein, *refpipe;

	refpipe = fopen("data", "r");
	pipein = fopen("samples_flipped", "r");

	uint8_t *data_buf = new uint8_t[2048];
	uint8_t *decode_buf = new uint8_t[2048];
	
	fread(data_buf, 1, 1024, refpipe);
	decode_bits(pipein, decode_buf, 1024);

	return memcmp(data_buf, decode_buf, 1024) ? 1 : 0;
}
