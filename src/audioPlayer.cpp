#ifndef UNICODE
#define UNICODE
#endif

#include "audioPlayer.hpp"
#include "yumo_except.hpp"
#include <cassert>
#include <functional>

namespace
{
    // 资源管理类，自动释放资源
    template <typename resourceType>
    class resourceManager
    {
    private:
        resourceType resource;         // 资源句柄或指针
        resourceType invalidValue;     // 资源无效值
        void (*deleter)(resourceType); // 资源释放函数指针

    public:
        resourceManager(
            resourceType res, resourceType invalid, void (*del)(resourceType))
            : resource(res), invalidValue(invalid), deleter(del) {}
        ~resourceManager()
        {
            if (resource != invalidValue)
                deleter(resource);
        }
        // 禁止拷贝，允许移动
        resourceManager(const resourceManager &) = delete;
        resourceManager &operator=(const resourceManager &) = delete;
        resourceType get() { return resource; }
    };

    // todo 支持WAVE_FORMAT_IEEE_FLOAT、WAVE_FORMAT_EXTENSIBLE等非PCM格式
    void ValidatePcmFormat(const WAVEFORMATEX &wf)
    {
        if (wf.wFormatTag != WAVE_FORMAT_PCM)
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"仅支持 PCM 格式");
        if (wf.nChannels != 1 && wf.nChannels != 2)
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"仅支持单声道或立体声");
        if (wf.wBitsPerSample != 8 && wf.wBitsPerSample != 16 && wf.wBitsPerSample != 32)
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"仅支持 8/16/32 位深度");
        if (wf.nSamplesPerSec < 8000 || wf.nSamplesPerSec > 192000)
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"采样率超出合理范围");
    }

    // 本文件内部常量表
    const DWORD RIFF_ID = 0x46464952; // "RIFF" 的小端序整数值
    const DWORD WAVE_ID = 0x45564157; // "WAVE" 的小端序整数值
    const DWORD FMT_ID = 0x20746D66;  // "fmt " 的小端序整数值（注意空格）
    const DWORD DATA_ID = 0x61746164; // "data" 的小端序整数值
    const size_t MINIMUM_FMT_SIZE = 16; // 最小fmt块大小（包含所有必要字段）
    const size_t MAXIMUM_FMT_SIZE = 24; // 最大fmt块大小（包含所有可选字段）
    const size_t COMMON_FMT_SIZE = sizeof(WAVEFORMATEX); // fmt块大小（包含所有字段）
}

namespace yumo
{
    // 从磁盘加载WAV文件，解析其格式块和数据块，填充WavInfo结构体
    void loadWav(const wchar_t *filename, WavInfo *out)
    {
        resourceManager<HANDLE> fileHandler(
            CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            INVALID_HANDLE_VALUE,
            [](HANDLE h) -> void
            { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); });

        // 文件打开失败
        if (fileHandler.get() == INVALID_HANDLE_VALUE)
            throw yumo::exception_ex(
                yumo::exception::type::FileOpenError,
                L"WAV文件打开失败，可能是文件不存在或其他错误");

        DWORD riff, fileLenMinus8, wave;
        DWORD bytesRead;
        // 读取RIFF头：RIFF标识、文件长度（减去8字节）、WAVE标识
        // 这个if可能有点长，嘻嘻
        if (!ReadFile(fileHandler.get(), &riff, 4, &bytesRead, NULL) || bytesRead != 4 ||
            !ReadFile(fileHandler.get(), &fileLenMinus8, 4, &bytesRead, NULL) || bytesRead != 4 ||
            !ReadFile(fileHandler.get(), &wave, 4, &bytesRead, NULL) || bytesRead != 4)
        {
            throw yumo::exception_ex(
                yumo::exception::type::FileReadError,
                L"WAV文件读取失败，可能是文件损坏或其他错误");
        }

        // 检查是否为合法的WAV文件（RIFF和WAVE标记）
        if (riff != RIFF_ID || wave != WAVE_ID)
        {
            throw yumo::exception_ex(
                yumo::exception::type::InvalidInput,
                L"WAV文件的RIFF或WAVE标记存在问题，可能不是合法的WAV文件");
        }

        DWORD chunkId, chunkSize;
        // 循环读取各个块，直到找到fmt和data块
        while (true)
        {
            if (!ReadFile(fileHandler.get(), &chunkId, 4, &bytesRead, NULL) || bytesRead != 4 ||
                !ReadFile(fileHandler.get(), &chunkSize, 4, &bytesRead, NULL) || bytesRead != 4)
            {
                throw yumo::exception_ex(
                    yumo::exception::type::FileReadError,
                    L"无法从WAV文件读取到fmt或data块，可能是文件损坏或格式异常");
            }
            if (chunkId == FMT_ID) // "fmt " （注意空格，别给我删咯） 音频格式块
            {
                if (chunkSize < MINIMUM_FMT_SIZE)
                {
                    throw yumo::exception_ex(
                        yumo::exception::type::InvalidInput,
                        L"WAV文件的格式块大小异常，无法解析音频格式信息");
                }
                // 先读取16字节的基本格式信息
                if (!ReadFile(fileHandler.get(), &out->wf, 16, &bytesRead, NULL) || bytesRead != 16)
                {
                    throw yumo::exception_ex(
                        yumo::exception::type::FileReadError,
                        L"WAV文件读取失败，可能是文件损坏或其他错误");
                }
                // 如果有扩展信息（chunkSize > MINIMUM_FMT_SIZE），读取cbSize
                if (chunkSize > MINIMUM_FMT_SIZE)
                {
                    if (!ReadFile(fileHandler.get(), &out->wf.cbSize, 2, &bytesRead, NULL) || bytesRead != 2)
                    {
                        throw yumo::exception_ex(
                            yumo::exception::type::FileReadError,
                            L"WAV文件读取失败，可能是文件损坏或其他错误");
                    }
                    // 如果有额外扩展信息，跳过它们（目前）
                    if (chunkSize > COMMON_FMT_SIZE)
                    {
                        if (SetFilePointer(fileHandler.get(), chunkSize - COMMON_FMT_SIZE, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
                        {
                            throw yumo::exception_ex(
                                yumo::exception::type::FileReadError,
                                L"WAV文件读取失败，可能是文件损坏或其他错误");
                        }
                    }
                }

                // 验证格式是否为支持的PCM格式
                ValidatePcmFormat(out->wf);
            }
            else if (chunkId == DATA_ID) // "data" 音频数据块
            {
                out->dataSize = chunkSize;
                // 分配内存并读取PCM数据到out->pcmData
                try
                {
                    out->pcmData = std::make_unique<char[]>(out->dataSize);
                }
                // 内存分配失败
                catch (const std::bad_alloc &)
                {
                    throw yumo::exception_ex(
                        yumo::exception::type::OutOfMemory,
                        L"内存分配失败，请检查WAV文件是否过大或受损");
                }
                // 读取PCM数据
                if (!ReadFile(fileHandler.get(), out->pcmData.get(), out->dataSize, &bytesRead, NULL) || bytesRead != out->dataSize)
                {
                    throw yumo::exception_ex(
                        yumo::exception::type::FileReadError,
                        L"WAV文件读取失败，可能是文件损坏或其他错误");
                }
                break; // 数据块读取完毕，退出循环
            }
            else
            {
                // todo GetLastError() != NO_ERROR
                // 忽略其他块（如list、fact等），直接跳过
                if (SetFilePointer(fileHandler.get(), chunkSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
                {
                    throw yumo::exception_ex(
                        yumo::exception::type::FileReadError,
                        L"WAV文件读取失败，可能是文件损坏或其他错误");
                }
            }
        }
        out->valid = true; // 标记结构体有效
    }

    // 使用 waveOut API 播放标准格式音频
    void playStandard(const StandardWavInfo &audioData)
    {
        // 标准格式：44.1kHz, 双声道, 16位
        WAVEFORMATEX wf = {};
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = 2;
        wf.nSamplesPerSec = 44100;
        wf.wBitsPerSample = 16;
        wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
        wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

        HWAVEOUT hWaveOut = NULL;
        MMRESULT mmResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);
        if (mmResult != MMSYSERR_NOERROR)
        {
            throw yumo::exception_ex(yumo::exception::type::UnknownError, L"波形音频设备打开失败");
        }

        auto closeWaveOut = [](HWAVEOUT h) { waveOutClose(h); };
        std::unique_ptr<std::remove_pointer_t<HWAVEOUT>, decltype(closeWaveOut)> 
            waveOutGuard(hWaveOut, closeWaveOut);

        WAVEHDR header = {};
        header.lpData = reinterpret_cast<LPSTR>(const_cast<int16_t*>(audioData.data()));
        header.dwBufferLength = static_cast<DWORD>(audioData.size() * sizeof(int16_t));

        mmResult = waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        if (mmResult != MMSYSERR_NOERROR)
        {
            throw yumo::exception_ex(yumo::exception::type::UnknownError, L"音频头准备失败");
        }

        std::unique_ptr<WAVEHDR, std::function<void(WAVEHDR*)>> 
            headerGuard(&header, [hWaveOut](WAVEHDR* hdr) { waveOutUnprepareHeader(hWaveOut, hdr, sizeof(WAVEHDR)); });

        mmResult = waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
        if (mmResult != MMSYSERR_NOERROR)
        {
            throw yumo::exception_ex(yumo::exception::type::UnknownError, L"音频播放失败");
        }

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(10);
        }
    }

    // 使用 libsamplerate 进行音频转换
    StandardWavInfo convertToStandard(const WavInfo &wavInfo)
    {
        // 检查WAV文件是否有效
        if (!wavInfo.valid)
        {
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"WAV文件格式无效");
        }

        const int sourceSampleRate = wavInfo.wf.nSamplesPerSec;
        const int sourceChannels = wavInfo.wf.nChannels;
        const int targetSampleRate = 44100;
        const int targetChannels = 2; // 双声道

        // 计算每个样本的字节数和总帧数
        const size_t bytesPerSample = wavInfo.wf.wBitsPerSample / 8;
        const size_t totalFrames = wavInfo.dataSize / (bytesPerSample * sourceChannels);

        // 步骤1：将原始数据转换为 float 格式（多声道交织）
        std::vector<float> inputFrames;
        inputFrames.reserve(totalFrames * sourceChannels);

        for (size_t i = 0; i < totalFrames * sourceChannels; ++i) {
            const char* samplePtr = wavInfo.pcmData.get() + (i * bytesPerSample);
            float normalizedValue = 0.0f;

            switch (wavInfo.wf.wBitsPerSample) {
                case 8: {
                    uint8_t sample = *reinterpret_cast<const uint8_t*>(samplePtr);
                    normalizedValue = (sample / 128.0f) - 1.0f;
                    break;
                }
                case 16: {
                    int16_t sample = *reinterpret_cast<const int16_t*>(samplePtr);
                    normalizedValue = sample / 32768.0f;
                    break;
                }
                case 24: {
                    int32_t sample = (
                        (static_cast<uint8_t>(samplePtr[0]) << 0) |
                        (static_cast<uint8_t>(samplePtr[1]) << 8) |
                        (static_cast<uint8_t>(samplePtr[2]) << 16)
                    );
                    if (sample & 0x800000) {
                        sample |= 0xFF000000;
                    }
                    normalizedValue = sample / 8388608.0f;
                    break;
                }
                case 32: {
                    int32_t sample = *reinterpret_cast<const int32_t*>(samplePtr);
                    normalizedValue = sample / 2147483648.0f;
                    break;
                }
                default:
                    throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"不支持的位深度");
            }
            inputFrames.push_back(normalizedValue);
        }

        // 步骤2：使用 libsamplerate 进行重采样
        const double ratio = static_cast<double>(targetSampleRate) / sourceSampleRate;
        const size_t outputFramesEstimate = static_cast<size_t>(totalFrames * ratio) + 1;
        
        std::vector<float> resampledFrames(outputFramesEstimate * sourceChannels);

        SRC_DATA srcData;
        srcData.data_in = inputFrames.data();
        srcData.data_out = resampledFrames.data();
        srcData.input_frames = static_cast<long>(totalFrames);
        srcData.output_frames = static_cast<long>(outputFramesEstimate);
        srcData.src_ratio = ratio;
        srcData.end_of_input = 1;

        int error = src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, sourceChannels);
        if (error != 0) {
            throw yumo::exception_ex(yumo::exception::type::UnknownError, L"重采样失败");
        }

        resampledFrames.resize(srcData.output_frames_gen * sourceChannels);

        // 步骤3：转换为双声道
        std::vector<float> stereoFrames;
        stereoFrames.reserve(srcData.output_frames_gen * targetChannels);

        if (sourceChannels == 1) {
            // 单声道转双声道：复制到两个声道
            const long outputFrames = srcData.output_frames_gen;
            for (long i = 0; i < outputFrames; ++i) {
                float sample = resampledFrames[i];
                stereoFrames.push_back(sample);
                stereoFrames.push_back(sample);
            }
        } else if (sourceChannels == 2) {
            stereoFrames = std::move(resampledFrames);
        } else {
            throw yumo::exception_ex(yumo::exception::type::InvalidInput, L"不支持超过2个声道");
        }

        // 步骤4：转换为 int16_t
        StandardWavInfo result;
        result.resize(stereoFrames.size());
        src_float_to_short_array(stereoFrames.data(), result.data(), static_cast<int>(stereoFrames.size()));

        return result;
    }
}