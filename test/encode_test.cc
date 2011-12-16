#include <sfsfssfsf.h>

int main(int argc, char *argv[])
{
	FILE *pipein, *pipeout;
	FILE *datapipe, *refpipe;

	datapipe = fopen("data", "r");
	pipeout = fopen("samples_flipped_2", "w");
	pipein = fopen("samples", "r");
	refpipe = fopen("samples_flipped", "r");

	uint8_t *encode_buf = (uint8_t *)malloc(2048);
	fread(encode_buf, 1, 2048, datapipe);
	encode_bits(pipein, pipeout, encode_buf, 2048);

	fclose(pipein);
	fclose(pipeout);
	pipeout = fopen("samples_flipped_2", "r");

	uint16_t *samples_flipped_2_buf = (uint16_t *)malloc(32768);
	uint16_t *samples_flipped_buf = (uint16_t *)malloc(32768);
	fread(samples_flipped_2_buf, 2, 16384, pipeout);
	fread(samples_flipped_buf, 2, 16384, refpipe);

	return memcmp(samples_flipped_buf, samples_flipped_2_buf, 32768) ? 1 : 0;
}
