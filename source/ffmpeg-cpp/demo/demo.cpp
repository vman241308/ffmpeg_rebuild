
#include <iostream>

#include <ffmpegcpp.h>

using namespace ffmpegcpp;
using namespace std;

uint8_t* generate_rgb(int width, int height, int pts, uint8_t *rgb)
{
	int x, y, cur;
	rgb = (uint8_t*)realloc(rgb, 4 * sizeof(uint8_t) * height * width);
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			cur = 4 * (y * width + x);
			rgb[cur + 0] = 0;
			rgb[cur + 1] = 0;
			rgb[cur + 2] = 0;
			rgb[cur + 3] = (pts % 25) * (255 / 25);
			if ((pts / 25) % 2 == 0)
			{
				if (y < height / 2)
				{
					if (x < width / 2)
					{
						/* Black. */
					}
					else
					{
						rgb[cur + 0] = 255;
					}
				}
				else
				{
					if (x < width / 2)
					{
						rgb[cur + 1] = 255;
					}
					else
					{
						rgb[cur + 2] = 255;
					}
				}
			}
			else
			{
				if (y < height / 2)
				{
					rgb[cur + 0] = 255;
					if (x < width / 2)
					{
						rgb[cur + 1] = 255;
					}
					else
					{
						rgb[cur + 2] = 255;
					}
				}
				else
				{
					if (x < width / 2)
					{
						rgb[cur + 1] = 255;
						rgb[cur + 2] = 255;
					}
					else
					{
						rgb[cur + 0] = 255;
						rgb[cur + 1] = 255;
						rgb[cur + 2] = 255;
					}
				}
			}
		}
	}
	return rgb;
}

int main(int argc, char **argv)
{
	char* inFilename = argv[1];
	char* inAudioFilename = argv[2];
	char* outFilename = argv[3];

	// create the output stream
	try
	{
		int width = 352;
		int height = 288;

		// create an audio encoder
		AudioCodec* audioCodec = new AudioCodec(AV_CODEC_ID_MP2);//AV_CODEC_ID_MP2

		// audio output stream
		OpenCodec* openAudioCodec = audioCodec->Open();
		AudioOutputStream* audioStream = new AudioOutputStream(openAudioCodec);
		AudioEncoder* audioEncoder = new AudioEncoder(audioStream);

		// create an audio source
		//RawFileSource* audioSource = new RawFileSource(inAudioFilename, AV_CODEC_ID_MP3, audioEncoder);
		Demuxer* demuxer = new Demuxer(inAudioFilename);
		AudioInputStream* audioSource = demuxer->GetBestAudioStream(audioEncoder);

		H264NVEncCodec* codec = new H264NVEncCodec(width, height, 30, AV_PIX_FMT_YUV420P);
		codec->SetPreset("hq");
		//PNGCodec* codec = new PNGCodec(width, height, 30);

		OpenCodec* openCodec = codec->Open();
		VideoOutputStream* stream = new VideoOutputStream(openCodec);

		// list of output streams for the muxer
		vector<OutputStream*> streams;
		//streams.push_back(stream);
		streams.push_back(audioStream);

		// create the encoder that will link the source and muxer together
		VideoEncoder* encoder = new VideoEncoder(stream);

		// create a filter
		VideoFilter* filter = new VideoFilter("alphaextract", encoder);


		// MP3: AV_CODEC_ID_MP3
		// WAV: AV_CODEC_ID_FIRST_AUDIO
		// VIDEO: AV_CODEC_ID_RAWVIDEO
		//RawFileSource* source = new RawFileSource(inFilename, AV_CODEC_ID_H264, encoder);
		RawVideoSource* source = new RawVideoSource(width, height, AV_PIX_FMT_RGBA, AV_PIX_FMT_RGBA, 30, filter);

		// create the output muxer
		Muxer* muxer = new Muxer(outFilename, streams);

		/*uint8_t *rgb = NULL;
		for (int i = 0; i < 100; ++i)
		{
			rgb = generate_rgb(width, height, i, rgb);
			source->WriteFrame(rgb, 4 * width);
		}*/

		//audioSource->Start();
		demuxer->Start();


		muxer->Close();


		// all done
		delete muxer;
		delete source;
		delete encoder;
		delete stream;
		delete codec;
	}
	catch (FFmpegException e)
	{
		cerr << e.what() << endl;
		throw e;
	}/*
	catch (...)
	{
		cout << "OMG! an unexpected exception has been caught" << endl;
	}*/
	catch (const char *c)
	{

	}

	cout << "Press any key to continue..." << endl;

	getchar();

	return 0;
}
