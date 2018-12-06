#include "AudioFormatConverter.h"
#include "FFmpegException.h"

namespace ffmpegcpp
{
	AudioFormatConverter::AudioFormatConverter(ConvertedAudioProcessor* writer, AVCodecContext* codecContext)
	{
		this->output = writer;
		this->codecContext = codecContext;

		if (converted_frame != nullptr)
		{
			av_frame_free(&converted_frame);
		}

		converted_frame = av_frame_alloc();
		int ret;
		if (!converted_frame)
		{
			CleanUp();
			throw FFmpegException("Error allocating an audio frame");
		}

		// calculate the sample count
		int nb_samples;
		if (codecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
			nb_samples = 10000;
		else
			nb_samples = codecContext->frame_size;

		// configure the frame based on our codec context
		converted_frame->format = codecContext->sample_fmt;
		converted_frame->channel_layout = codecContext->channel_layout;
		converted_frame->sample_rate = codecContext->sample_rate;
		converted_frame->nb_samples = nb_samples;
		if (nb_samples)
		{
			ret = av_frame_get_buffer(converted_frame, 0);
			if (ret < 0)
			{
				CleanUp();
				throw FFmpegException("Error allocating an audio buffer", ret);
			}
		}
	}

	AudioFormatConverter::~AudioFormatConverter()
	{
		CleanUp();
	}

	void AudioFormatConverter::CleanUp()
	{
		if (converted_frame != nullptr)
		{
			av_frame_free(&converted_frame);
			converted_frame = nullptr;
		}
		if (swr_ctx != nullptr)
		{
			swr_free(&swr_ctx);
			swr_ctx = nullptr;
		}
	}

	void AudioFormatConverter::InitDelayed(AVFrame* frame)
	{
		swr_ctx = swr_alloc();
		if (!swr_ctx)
		{
			throw FFmpegException("Could not allocate resampler context");
		}

		// set options
		in_sample_rate = frame->sample_rate;
		out_sample_rate = codecContext->sample_rate;
		av_opt_set_int(swr_ctx, "in_channel_count", frame->channels, 0);
		av_opt_set_int(swr_ctx, "in_sample_rate", frame->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (AVSampleFormat)frame->format, 0);
		av_opt_set_int(swr_ctx, "out_channel_count", codecContext->channels, 0);
		av_opt_set_int(swr_ctx, "out_sample_rate", codecContext->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", codecContext->sample_fmt, 0);

		// initialize the resampling context
		int ret;
		if ((ret = swr_init(swr_ctx)) < 0)
		{
			throw FFmpegException("Failed to initialize the resampling context", ret);
		}

	}

	void AudioFormatConverter::ProcessFrame(AVFrame* frame)
	{
		// initialize the resampler
		if (!initialized)
		{
			InitDelayed(frame);
			initialized = true;
		}

		int ret;

		if (frame->pts == 6174)
		{
			int x = 3;
		}


		int64_t or_delay = swr_get_delay(swr_ctx, in_sample_rate);
		int64_t or_dst_nb_samples = av_rescale_rnd(or_delay,
			out_sample_rate, in_sample_rate, AV_ROUND_DOWN);

		ret = swr_convert_frame(swr_ctx, NULL, frame);
		if (ret < 0)
		{
			throw FFmpegException("Error while converting audio frame to destination format", ret);
		}

		// we don't need the old frame anymore
		//av_frame_unref(frame);

		// Because the frames might not be aligned, we wait until there are enough samples,
		// to fill a frame of the right size for the encoder. Because of differences in framerate,
		// multiple converted_frames might come out of one input frame, or multiple input frames
		// might fit into one converted_frame.

		int64_t delay = swr_get_delay(swr_ctx, in_sample_rate);
		int64_t dst_nb_samples = av_rescale_rnd(delay,
			out_sample_rate, in_sample_rate, AV_ROUND_DOWN);

		while (dst_nb_samples > converted_frame->nb_samples)
		{
			// when we pass a frame to the encoder, it may keep a reference to it
			// internally;
			// make sure we do not overwrite it here
			ret = av_frame_make_writable(converted_frame);
			if (ret < 0)
			{
				throw FFmpegException("Failed to make audio frame writable", ret);
			}

			int original_nb_samples = converted_frame->nb_samples;
			ret = swr_convert_frame(swr_ctx, converted_frame, NULL);
			if (original_nb_samples != converted_frame->nb_samples)
			{
				int x = 5;
				return;
			}
			if (ret < 0)
			{
				throw FFmpegException("Error while converting audio frame to destination format", ret);
			}

			WriteCompleteConvertedFrame();

			delay = swr_get_delay(swr_ctx, in_sample_rate);
			dst_nb_samples = av_rescale_rnd(delay,
				out_sample_rate, in_sample_rate, AV_ROUND_DOWN);
		}
		return;
	}

	void AudioFormatConverter::WriteCompleteConvertedFrame()
	{
		AVRational inv_sample_rate;
		inv_sample_rate.num = 1;
		inv_sample_rate.den = codecContext->sample_rate;

		converted_frame->pts = av_rescale_q(samples_count, inv_sample_rate, codecContext->time_base);
		samples_count += converted_frame->nb_samples;

		output->WriteConvertedFrame(converted_frame);

		samplesInCurrentFrame = 0;
		int ret = av_frame_make_writable(converted_frame);
		if (ret < 0)
		{
			throw FFmpegException("Failed to make audio frame writable", ret);
		}

	}

}

