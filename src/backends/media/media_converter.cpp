// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/media/media_converter.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace morpho::media {
namespace {

constexpr std::uint64_t kMaxOutputBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;
constexpr int kOutputSampleRate = 48000;
constexpr int64_t kVideoBitrate = 2'000'000;
constexpr int64_t kAudioBitrate = 192'000;

struct OutputSpec {
    const char *muxer;
    const char *extension;
    const char *videoCodec;
    const char *audioCodec;
    bool hasVideo;
    bool hasAudio;
};

std::optional<OutputSpec> outputSpec(const std::string &format) {
    if (format == "mp3") {
        return OutputSpec{"mp3", "mp3", nullptr, "libmp3lame", false, true};
    }
    if (format == "m4a") {
        return OutputSpec{"mp4", "m4a", nullptr, "aac", false, true};
    }
    if (format == "opus") {
        return OutputSpec{"ogg", "opus", nullptr, "libopus", false, true};
    }
    if (format == "vorbis") {
        return OutputSpec{"ogg", "ogg", nullptr, "libvorbis", false, true};
    }
    if (format == "flac") {
        return OutputSpec{"flac", "flac", nullptr, "flac", false, true};
    }
    if (format == "wav") {
        return OutputSpec{"wav", "wav", nullptr, "pcm_s16le", false, true};
    }
    if (format == "mp4") {
        return OutputSpec{"mp4", "mp4", "mpeg4", "aac", true, true};
    }
    if (format == "webm") {
        return OutputSpec{"webm", "webm", "libvpx-vp9", "libopus", true, true};
    }
    if (format == "mkv") {
        return OutputSpec{"matroska", "mkv", "libvpx-vp9", "libopus", true, true};
    }
    if (format == "mov") {
        return OutputSpec{"mov", "mov", "mpeg4", "aac", true, true};
    }
    if (format == "gif") {
        return OutputSpec{"gif", "gif", "gif", nullptr, true, false};
    }
    return std::nullopt;
}

std::string errorText(int code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(code, buffer, sizeof(buffer));
    return buffer;
}

std::string upperExtension(const std::filesystem::path &path) {
    std::string extension = path.extension().string();
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    std::transform(
        extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    return extension.empty() ? "MEDIA" : extension;
}

struct MemoryOutput {
    std::vector<std::uint8_t> data;
    AVIOContext *context = nullptr;
    std::int64_t position = 0;

    ~MemoryOutput() {
        if (context != nullptr) {
            av_freep(&context->buffer);
            avio_context_free(&context);
        }
    }
};

int writeMemory(void *opaque, std::uint8_t *buffer, int size) {
    auto *output = static_cast<MemoryOutput *>(opaque);
    if (size < 0) {
        return AVERROR(EINVAL);
    }

    const auto count = static_cast<std::size_t>(size);
    if (output->position < 0 ||
        static_cast<std::uint64_t>(output->position) + count > kMaxOutputBytes) {
        return AVERROR(ENOSPC);
    }

    const auto position = static_cast<std::size_t>(output->position);
    if (position + count > output->data.size()) {
        output->data.resize(position + count);
    }
    std::memcpy(output->data.data() + position, buffer, count);
    output->position += static_cast<std::int64_t>(count);
    return size;
}

std::int64_t seekMemory(void *opaque, std::int64_t offset, int whence) {
    auto *output = static_cast<MemoryOutput *>(opaque);
    if (whence == AVSEEK_SIZE) {
        return static_cast<std::int64_t>(output->data.size());
    }

    std::int64_t base = 0;
    if (whence == SEEK_CUR) {
        base = output->position;
    } else if (whence == SEEK_END) {
        base = static_cast<std::int64_t>(output->data.size());
    } else if (whence != SEEK_SET) {
        return AVERROR(EINVAL);
    }

    const std::int64_t next = base + offset;
    if (next < 0) {
        return AVERROR(EINVAL);
    }
    output->position = next;
    return next;
}

struct Decoder {
    AVCodecContext *context = nullptr;
    AVStream *stream = nullptr;

    ~Decoder() {
        avcodec_free_context(&context);
    }
};

struct Encoder {
    AVCodecContext *context = nullptr;
    AVStream *stream = nullptr;

    ~Encoder() {
        avcodec_free_context(&context);
    }
};

struct SwrState {
    SwrContext *context = nullptr;

    ~SwrState() {
        swr_free(&context);
    }
};

struct SwsState {
    SwsContext *context = nullptr;

    ~SwsState() {
        sws_freeContext(context);
    }
};

struct AudioFifoState {
    AVAudioFifo *fifo = nullptr;

    ~AudioFifoState() {
        av_audio_fifo_free(fifo);
    }
};

std::unique_ptr<Decoder> openDecoder(AVStream *stream, std::string &error) {
    auto decoder = std::make_unique<Decoder>();
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        error = "No decoder is available for this stream.";
        return nullptr;
    }

    decoder->context = avcodec_alloc_context3(codec);
    if (decoder->context == nullptr ||
        avcodec_parameters_to_context(decoder->context, stream->codecpar) < 0 ||
        avcodec_open2(decoder->context, codec, nullptr) < 0) {
        error = "Could not open the input decoder.";
        return nullptr;
    }

    decoder->stream = stream;
    return decoder;
}

std::optional<AVSampleFormat> chooseSampleFormat(const AVCodec *codec) {
    if (codec->sample_fmts == nullptr) {
        return std::nullopt;
    }
    for (const AVSampleFormat *format = codec->sample_fmts; *format != AV_SAMPLE_FMT_NONE;
         ++format) {
        return *format;
    }
    return std::nullopt;
}

std::optional<AVPixelFormat> choosePixelFormat(const AVCodec *codec, const char *name) {
    const AVPixelFormat preferred =
        std::strcmp(name, "gif") == 0 ? AV_PIX_FMT_RGB8 : AV_PIX_FMT_YUV420P;
    if (codec->pix_fmts != nullptr) {
        for (const AVPixelFormat *format = codec->pix_fmts; *format != AV_PIX_FMT_NONE; ++format) {
            if (*format == preferred) {
                return preferred;
            }
        }
        for (const AVPixelFormat *format = codec->pix_fmts; *format != AV_PIX_FMT_NONE; ++format) {
            return *format;
        }
    }
    return std::nullopt;
}

AVRational frameRate(const AVFormatContext *input, AVStream *stream) {
    AVRational rate = av_guess_frame_rate(const_cast<AVFormatContext *>(input), stream, nullptr);
    if (rate.num <= 0 || rate.den <= 0) {
        rate = AVRational{30, 1};
    }
    return rate;
}

} // namespace

bool MediaConverter::supportsInput(const std::filesystem::path &path) const {
    AVFormatContext *raw = nullptr;
    if (avformat_open_input(&raw, path.string().c_str(), nullptr, nullptr) < 0) {
        return false;
    }

    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)> input(
        raw, [](AVFormatContext *context) {
            if (context != nullptr) {
                avformat_close_input(&context);
            }
        });

    if (avformat_find_stream_info(input.get(), nullptr) < 0) {
        return false;
    }

    for (unsigned int i = 0; i < input->nb_streams; ++i) {
        const AVMediaType type = input->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_AUDIO || type == AVMEDIA_TYPE_VIDEO) {
            return true;
        }
    }
    return false;
}

ConversionResult MediaConverter::convert(const ConversionRequest &request) {
    ConversionResult result;
    result.inputPath = request.inputPath;
    result.inputFormat = upperExtension(request.inputPath);
    result.outputFormat = request.outputFormat;

    std::error_code sizeError;
    const std::uintmax_t inputSize = std::filesystem::file_size(request.inputPath, sizeError);
    if (!sizeError) {
        result.inputBytes = static_cast<std::uint64_t>(inputSize);
    }

    const std::optional<OutputSpec> spec = outputSpec(request.outputFormat);
    if (!spec.has_value()) {
        result.error = "Unsupported audio or video output format.";
        return result;
    }

    AVFormatContext *inputRaw = nullptr;
    const int openResult =
        avformat_open_input(&inputRaw, request.inputPath.string().c_str(), nullptr, nullptr);
    if (openResult < 0) {
        result.error = "Could not open the input media: " + errorText(openResult);
        return result;
    }

    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)> input(
        inputRaw, [](AVFormatContext *context) {
            if (context != nullptr) {
                avformat_close_input(&context);
            }
        });

    if (avformat_find_stream_info(input.get(), nullptr) < 0) {
        result.error = "Could not read the input media streams.";
        return result;
    }

    const int videoIndex =
        spec->hasVideo ? av_find_best_stream(input.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)
                       : AVERROR_STREAM_NOT_FOUND;
    const int audioIndex =
        spec->hasAudio ? av_find_best_stream(input.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)
                       : AVERROR_STREAM_NOT_FOUND;

    if (spec->hasVideo && videoIndex < 0) {
        result.error = "The input media has no video stream.";
        return result;
    }
    if (spec->hasAudio && !spec->hasVideo && audioIndex < 0) {
        result.error = "The input media has no audio stream.";
        return result;
    }

    std::unique_ptr<Decoder> videoDecoder;
    std::unique_ptr<Decoder> audioDecoder;
    if (videoIndex >= 0) {
        videoDecoder = openDecoder(input->streams[videoIndex], result.error);
        if (videoDecoder == nullptr) {
            return result;
        }
    }
    if (audioIndex >= 0) {
        audioDecoder = openDecoder(input->streams[audioIndex], result.error);
        if (audioDecoder == nullptr) {
            return result;
        }
    }

    MemoryOutput memory;

    AVFormatContext *outputRaw = nullptr;
    const int allocateResult =
        avformat_alloc_output_context2(&outputRaw, nullptr, spec->muxer, nullptr);
    if (allocateResult < 0 || outputRaw == nullptr) {
        result.error = "Could not create the output container: " + errorText(allocateResult);
        return result;
    }

    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)> output(
        outputRaw, [](AVFormatContext *context) { avformat_free_context(context); });

    Encoder videoEncoder;
    Encoder audioEncoder;
    SwsState sws;
    SwrState swr;
    AudioFifoState audioFifo;
    int outputWidth = 0;
    int outputHeight = 0;
    AVPixelFormat outputPixelFormat = AV_PIX_FMT_NONE;
    AVSampleFormat outputSampleFormat = AV_SAMPLE_FMT_NONE;
    AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;

    if (videoDecoder != nullptr) {
        const AVCodec *codec = avcodec_find_encoder_by_name(spec->videoCodec);
        if (codec == nullptr) {
            result.error = "The required video encoder is not available.";
            return result;
        }

        const std::optional<AVPixelFormat> pixelFormat = choosePixelFormat(codec, spec->videoCodec);
        if (!pixelFormat.has_value()) {
            result.error = "The video encoder has no usable pixel format.";
            return result;
        }
        outputPixelFormat = *pixelFormat;
        outputWidth = videoDecoder->context->width;
        outputHeight = videoDecoder->context->height;
        if (outputWidth <= 0 || outputHeight <= 0) {
            result.error = "The input video has invalid dimensions.";
            return result;
        }
        if (outputPixelFormat == AV_PIX_FMT_YUV420P) {
            outputWidth &= ~1;
            outputHeight &= ~1;
        }
        if (outputWidth <= 0 || outputHeight <= 0) {
            result.error = "The output video dimensions are invalid.";
            return result;
        }

        videoEncoder.context = avcodec_alloc_context3(codec);
        if (videoEncoder.context == nullptr) {
            result.error = "Could not allocate the video encoder.";
            return result;
        }

        const AVRational rate = frameRate(input.get(), videoDecoder->stream);
        videoEncoder.context->width = outputWidth;
        videoEncoder.context->height = outputHeight;
        videoEncoder.context->pix_fmt = outputPixelFormat;
        videoEncoder.context->time_base = av_inv_q(rate);
        videoEncoder.context->framerate = rate;
        videoEncoder.context->bit_rate = kVideoBitrate;
        videoEncoder.context->gop_size = 12;
        videoEncoder.context->max_b_frames = 0;
        videoEncoder.context->thread_count = 1;
        if ((output->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
            videoEncoder.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(videoEncoder.context, codec, nullptr) < 0) {
            result.error = "Could not open the video encoder.";
            return result;
        }

        videoEncoder.stream = avformat_new_stream(output.get(), nullptr);
        if (videoEncoder.stream == nullptr ||
            avcodec_parameters_from_context(videoEncoder.stream->codecpar, videoEncoder.context) <
                0) {
            result.error = "Could not create the output video stream.";
            return result;
        }
        videoEncoder.stream->time_base = videoEncoder.context->time_base;

        sws.context = sws_getContext(videoDecoder->context->width, videoDecoder->context->height,
                                     videoDecoder->context->pix_fmt, outputWidth, outputHeight,
                                     outputPixelFormat, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (sws.context == nullptr) {
            result.error = "Could not create the video scaler.";
            return result;
        }
    }

    if (audioDecoder != nullptr) {
        const AVCodec *codec = avcodec_find_encoder_by_name(spec->audioCodec);
        if (codec == nullptr) {
            result.error = "The required audio encoder is not available.";
            return result;
        }

        const std::optional<AVSampleFormat> sampleFormat = chooseSampleFormat(codec);
        if (!sampleFormat.has_value()) {
            result.error = "The audio encoder has no usable sample format.";
            return result;
        }
        outputSampleFormat = *sampleFormat;

        audioEncoder.context = avcodec_alloc_context3(codec);
        if (audioEncoder.context == nullptr) {
            result.error = "Could not allocate the audio encoder.";
            return result;
        }

        audioEncoder.context->sample_rate = kOutputSampleRate;
        audioEncoder.context->sample_fmt = outputSampleFormat;
        av_channel_layout_copy(&audioEncoder.context->ch_layout, &stereoLayout);
        audioEncoder.context->time_base = AVRational{1, kOutputSampleRate};
        audioEncoder.context->bit_rate = (std::strcmp(spec->audioCodec, "flac") == 0 ||
                                          std::strcmp(spec->audioCodec, "pcm_s16le") == 0)
                                             ? 0
                                             : kAudioBitrate;
        if ((output->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
            audioEncoder.context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(audioEncoder.context, codec, nullptr) < 0) {
            result.error = "Could not open the audio encoder.";
            return result;
        }

        audioEncoder.stream = avformat_new_stream(output.get(), nullptr);
        if (audioEncoder.stream == nullptr ||
            avcodec_parameters_from_context(audioEncoder.stream->codecpar, audioEncoder.context) <
                0) {
            result.error = "Could not create the output audio stream.";
            return result;
        }
        audioEncoder.stream->time_base = audioEncoder.context->time_base;

        AVChannelLayout inputLayout{};
        if (av_channel_layout_check(&audioDecoder->context->ch_layout)) {
            av_channel_layout_copy(&inputLayout, &audioDecoder->context->ch_layout);
        } else {
            const int channels = audioDecoder->context->ch_layout.nb_channels > 0
                                     ? audioDecoder->context->ch_layout.nb_channels
                                     : 2;
            av_channel_layout_default(&inputLayout, channels);
        }

        const int swrResult = swr_alloc_set_opts2(
            &swr.context, &stereoLayout, outputSampleFormat, kOutputSampleRate, &inputLayout,
            audioDecoder->context->sample_fmt, audioDecoder->context->sample_rate, 0, nullptr);
        av_channel_layout_uninit(&inputLayout);
        if (swrResult < 0 || swr_init(swr.context) < 0) {
            result.error = "Could not create the audio resampler.";
            return result;
        }

        audioFifo.fifo = av_audio_fifo_alloc(outputSampleFormat, 2, 1);
        if (audioFifo.fifo == nullptr) {
            result.error = "Could not create the audio buffer.";
            return result;
        }
    }

    auto *avioBuffer = static_cast<unsigned char *>(av_malloc(32768));
    if (avioBuffer == nullptr) {
        result.error = "Could not allocate the output buffer.";
        return result;
    }

    memory.context =
        avio_alloc_context(avioBuffer, 32768, 1, &memory, nullptr, writeMemory, seekMemory);
    if (memory.context == nullptr) {
        av_free(avioBuffer);
        result.error = "Could not create the output IO context.";
        return result;
    }
    memory.context->seekable = AVIO_SEEKABLE_NORMAL;
    output->pb = memory.context;
    output->flags |= AVFMT_FLAG_CUSTOM_IO;

    const int headerResult = avformat_write_header(output.get(), nullptr);
    if (headerResult < 0) {
        result.error = "Could not write the output header: " + errorText(headerResult);
        return result;
    }

    int64_t nextAudioPts = 0;
    int64_t nextVideoPts = 0;
    std::string encodeError;

    auto muxPackets = [&](Encoder &encoder) {
        AVPacket *packet = av_packet_alloc();
        if (packet == nullptr) {
            encodeError = "Could not allocate an output packet.";
            return false;
        }

        while (true) {
            const int receiveResult = avcodec_receive_packet(encoder.context, packet);
            if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                break;
            }
            if (receiveResult < 0) {
                encodeError = "Could not encode a media packet: " + errorText(receiveResult);
                av_packet_free(&packet);
                return false;
            }

            av_packet_rescale_ts(packet, encoder.context->time_base, encoder.stream->time_base);
            packet->stream_index = encoder.stream->index;
            const int writeResult = av_interleaved_write_frame(output.get(), packet);
            av_packet_unref(packet);
            if (writeResult < 0) {
                encodeError = "Could not write a media packet: " + errorText(writeResult);
                av_packet_free(&packet);
                return false;
            }
        }

        av_packet_free(&packet);
        return true;
    };

    auto sendFrame = [&](Encoder &encoder, AVFrame *frame) {
        const int sendResult = avcodec_send_frame(encoder.context, frame);
        if (sendResult < 0) {
            encodeError = "Could not submit a frame to the encoder: " + errorText(sendResult);
            return false;
        }
        return muxPackets(encoder);
    };

    auto emitAudioSamples = [&](int samples) {
        AVFrame *outputFrame = av_frame_alloc();
        if (outputFrame == nullptr) {
            encodeError = "Could not allocate an audio frame.";
            return false;
        }
        outputFrame->format = outputSampleFormat;
        outputFrame->sample_rate = kOutputSampleRate;
        outputFrame->nb_samples = samples;
        av_channel_layout_copy(&outputFrame->ch_layout, &stereoLayout);

        if (av_frame_get_buffer(outputFrame, 0) < 0) {
            encodeError = "Could not allocate the audio buffer.";
            av_frame_free(&outputFrame);
            return false;
        }

        if (av_audio_fifo_read(audioFifo.fifo, reinterpret_cast<void **>(outputFrame->data),
                               samples) < samples) {
            encodeError = "Could not read the audio buffer.";
            av_frame_free(&outputFrame);
            return false;
        }

        outputFrame->pts = nextAudioPts;
        nextAudioPts += samples;
        const bool sent = sendFrame(audioEncoder, outputFrame);
        av_frame_free(&outputFrame);
        return sent;
    };

    auto processAudioFrame = [&](AVFrame *frame) {
        const int maxSamples = swr_get_out_samples(swr.context, frame->nb_samples);
        if (maxSamples < 0) {
            encodeError = "Could not calculate the audio output size.";
            return false;
        }

        AVFrame *convertedFrame = av_frame_alloc();
        if (convertedFrame == nullptr) {
            encodeError = "Could not allocate an audio frame.";
            return false;
        }
        convertedFrame->format = outputSampleFormat;
        convertedFrame->sample_rate = kOutputSampleRate;
        convertedFrame->nb_samples = maxSamples;
        av_channel_layout_copy(&convertedFrame->ch_layout, &stereoLayout);

        if (av_frame_get_buffer(convertedFrame, 0) < 0) {
            encodeError = "Could not allocate the audio buffer.";
            av_frame_free(&convertedFrame);
            return false;
        }

        const int converted =
            swr_convert(swr.context, convertedFrame->data, maxSamples,
                        const_cast<const std::uint8_t **>(frame->extended_data), frame->nb_samples);
        if (converted < 0) {
            encodeError = "Could not resample the audio.";
            av_frame_free(&convertedFrame);
            return false;
        }

        convertedFrame->nb_samples = converted;
        if (av_audio_fifo_write(audioFifo.fifo, reinterpret_cast<void **>(convertedFrame->data),
                                converted) < converted) {
            encodeError = "Could not buffer the audio samples.";
            av_frame_free(&convertedFrame);
            return false;
        }
        av_frame_free(&convertedFrame);

        int frameSize = audioEncoder.context->frame_size;
        if (frameSize <= 0) {
            frameSize = 1024;
        }
        while (av_audio_fifo_size(audioFifo.fifo) >= frameSize) {
            if (!emitAudioSamples(frameSize)) {
                return false;
            }
        }
        return true;
    };

    auto processVideoFrame = [&](AVFrame *frame) {
        AVFrame *outputFrame = av_frame_alloc();
        if (outputFrame == nullptr) {
            encodeError = "Could not allocate a video frame.";
            return false;
        }
        outputFrame->format = outputPixelFormat;
        outputFrame->width = outputWidth;
        outputFrame->height = outputHeight;

        if (av_frame_get_buffer(outputFrame, 0) < 0) {
            encodeError = "Could not allocate the video buffer.";
            av_frame_free(&outputFrame);
            return false;
        }

        sws_scale(sws.context, frame->data, frame->linesize, 0, videoDecoder->context->height,
                  outputFrame->data, outputFrame->linesize);

        outputFrame->pts = nextVideoPts++;

        const bool sent = sendFrame(videoEncoder, outputFrame);
        av_frame_free(&outputFrame);
        return sent;
    };

    AVPacket *inputPacket = av_packet_alloc();
    AVFrame *decodedFrame = av_frame_alloc();
    if (inputPacket == nullptr || decodedFrame == nullptr) {
        av_packet_free(&inputPacket);
        av_frame_free(&decodedFrame);
        result.error = "Could not allocate media decode buffers.";
        return result;
    }

    auto drainDecoder = [&](Decoder &decoder, const std::function<bool(AVFrame *)> &process) {
        while (true) {
            const int receiveResult = avcodec_receive_frame(decoder.context, decodedFrame);
            if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
                break;
            }
            if (receiveResult < 0) {
                encodeError = "Could not decode a media frame: " + errorText(receiveResult);
                return false;
            }
            if (!process(decodedFrame)) {
                av_frame_unref(decodedFrame);
                return false;
            }
            av_frame_unref(decodedFrame);
        }
        return true;
    };

    auto decodePacket = [&](Decoder &decoder, const AVPacket *packet,
                            const std::function<bool(AVFrame *)> &process) {
        const int sendResult = avcodec_send_packet(decoder.context, packet);
        if (sendResult < 0) {
            encodeError = "Could not submit a packet to the decoder: " + errorText(sendResult);
            return false;
        }
        return drainDecoder(decoder, process);
    };

    while (true) {
        const int readResult = av_read_frame(input.get(), inputPacket);
        if (readResult == AVERROR_EOF) {
            break;
        }
        if (readResult < 0) {
            encodeError = "Could not read the input media: " + errorText(readResult);
            break;
        }

        bool ok = true;
        if (audioDecoder != nullptr && inputPacket->stream_index == audioIndex) {
            ok = decodePacket(*audioDecoder, inputPacket, processAudioFrame);
        } else if (videoDecoder != nullptr && inputPacket->stream_index == videoIndex) {
            ok = decodePacket(*videoDecoder, inputPacket, processVideoFrame);
        }
        av_packet_unref(inputPacket);

        if (!ok) {
            break;
        }
    }

    if (encodeError.empty() && audioDecoder != nullptr) {
        avcodec_send_packet(audioDecoder->context, nullptr);
        drainDecoder(*audioDecoder, processAudioFrame);
    }
    if (encodeError.empty() && videoDecoder != nullptr) {
        avcodec_send_packet(videoDecoder->context, nullptr);
        drainDecoder(*videoDecoder, processVideoFrame);
    }
    if (encodeError.empty() && audioDecoder != nullptr) {
        const int remaining = av_audio_fifo_size(audioFifo.fifo);
        if (remaining > 0) {
            emitAudioSamples(remaining);
        }
        sendFrame(audioEncoder, nullptr);
    }
    if (encodeError.empty() && videoDecoder != nullptr) {
        sendFrame(videoEncoder, nullptr);
    }

    av_packet_free(&inputPacket);
    av_frame_free(&decodedFrame);

    if (!encodeError.empty()) {
        result.error = std::move(encodeError);
        return result;
    }

    const int trailerResult = av_write_trailer(output.get());
    if (trailerResult < 0) {
        result.error = "Could not finish the output media: " + errorText(trailerResult);
        return result;
    }

    result.success = true;
    result.data = std::move(memory.data);

    std::filesystem::path outputName = request.inputPath.filename();
    outputName.replace_extension(std::string(".") + spec->extension);
    result.outputFileName = outputName.string();
    return result;
}

} // namespace morpho::media
