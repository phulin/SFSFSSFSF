#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define BUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

#define err(msg) { fprintf(stderr, msg); exit(1); }

int main(int argc, char *argv[]) {
	AVCodec *codec;
	AVCodecContext *c = NULL;
	AVFormatContext *fc = NULL;
	uint8_t *outbuf;
	uint8_t inbuf[BUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
	AVPacket avpkt;
	int len, out_size;
	FILE *f;

	if (argc < 1) err("Not enough arguments.\n");

	// initialization
	av_register_all();
	avcodec_init();

	// Open file
	if (avformat_open_input(&fc, argv[1], NULL, NULL))
		err("Couldn't open file.\n");

	// Stream info
	//if (avformat_find_stream_info(&fc, NULL) < 0)
	//	err("Couldn't get stream info.\n");
	
	dump_format(fc, 0, argv[1], 0);

	// Assume it has two streams for now
	c = fc->streams[0]->codec;
	if (c == NULL) err("Couldn't get codec.\n");

	av_init_packet(&avpkt);

	// find codec
	codec = avcodec_find_decoder(CODEC_ID_ALAC);
	if (!codec) err("Couldn't find codec\n");

	c = avcodec_alloc_context();

	// open codec
	if (avcodec_open(c, codec) < 0) err("Couldn't open codec.\n");

	av_read_frame(fc, &avpkt);

	av_pkt_dump2(stderr, &avpkt, true, fc->streams[0]);

	/*
	outbuf = malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);

	// ready to read
	avpkt.data = inbuf;
	avpkt.size = fread(inbuf, 1, BUF_SIZE, f);

	while (avpkt.size > 0) {
		out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		len = avcodec_decode_audio3(c, (short *) outbuf, &out_size, &avpkt);

		if (len < 0) err("Error while decoding.\n");

		if (out_size > 0) {
			int i;
			for (i = 0; i < out_size; i++)
			fwrite(outbuf, 1, out_size, stdout);
		}

		avpkt.size -= len;
		avpkt.data += len;
		if (avpkt.size < AUDIO_REFILL_THRESH) {
			// refill input buffer
			memmove(inbuf, avpkt.data, avpkt.size);
			avpkt.data = inbuf;
			len = fread(avpkt.data + avpkt.size, 1, BUF_SIZE - avpkt.size, stdin);
			if (len > 0)
				avpkt.size += len;
		}
	}
	*/

	free(outbuf);
	avcodec_close(c);
	av_close_input_file(fc);
}
