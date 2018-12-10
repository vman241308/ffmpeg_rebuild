
#include <iostream>

#include "ffmpegcpp.h"

using namespace std;
using namespace ffmpegcpp;

int main()
{
	// This example will apply some filters to a video and write it back.
	try
	{
		// Create a muxer that will output the video as MKV.
		Muxer* muxer = new Muxer("filtered_video.mp4");

		// Create a MP3 codec that will encode the raw data.
		VideoCodec* codec = new VideoCodec(AV_CODEC_ID_MPEG2VIDEO);
		codec->SetOption("preset", "default");
		//VideoCodec* codec = new H264NVEncCodec();

		// Create an encoder that will encode the raw audio data as MP3.
		// Tie it to the muxer so it will be written to the file.
		VideoEncoder* encoder = new VideoEncoder(codec, muxer);

		// Load the raw video file so we can process it.
		// FFmpeg is very good at deducing the file format, even from raw video files,
		// but if we have something weird, we can specify the properties of the format
		// in the constructor as commented out below.
		RawVideoFileSource* videoFile = new RawVideoFileSource("samples/carphone_qcif.y4m", encoder);

		// Prepare the output pipeline. This will push a small amount of frames to the file sink until it IsPrimed returns true.
		videoFile->PreparePipeline();

		// Push all the remaining frames through.
		while (!videoFile->IsDone())
		{
			videoFile->Step();
		}
		
		// Save everything to disk by closing the muxer.
		muxer->Close();
	}
	catch (const char* bla)
	{

	}
	/*catch (FFmpegException e)
	{
		cerr << "Exception caught!" << endl;
		cerr << e.what() << endl;
		throw e;
	}*/

	cout << "Encoding complete!" << endl;
	cout << "Press any key to continue..." << endl;

	getchar();
}
