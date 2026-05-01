#include "../src/audioPlayer.hpp"
#include "../src/yumo_except.hpp"
#include <iostream>
#include <io.h>
#include <fcntl.h>

int main()
{
    #ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
    #endif

    yumo::WavInfo info;
    try
    {
        std::wcout << L"正在加载WAV文件..." << std::endl;
        yumo::loadWav(L"./audio/test.wav", &info);
        std::wcout << L"WAV文件加载完成，正在显示信息..." << std::endl;
        if (info.valid)
        {
            std::wcout << L"WAV文件加载成功！" << std::endl;
            std::wcout << L"采样率: " << info.wf.nSamplesPerSec << L" Hz" << std::endl;
            std::wcout << L"声道数: " << info.wf.nChannels << std::endl;
            std::wcout << L"位深度: " << info.wf.wBitsPerSample << L" bits" << std::endl;
            std::wcout << L"PCM数据大小: " << info.dataSize << L" bytes" << std::endl;
        }
        else
        {
            std::wcout << L"WAV文件加载失败，信息无效。" << std::endl;
        }
    }
    catch (const yumo::exception_ex &e)
    {
        std::wcout << L"发生异常: " << e.what() << std::endl;
    }

    std::wcin.get();

    return 0;
}