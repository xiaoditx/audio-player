#include "../src/audioPlayer.hpp"
#include "../src/yumo_except.hpp"
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <string>

void testAudioFile(const wchar_t* filename)
{
    try
    {
        std::wcout << L"\n=== 测试文件: " << filename << L" ===" << std::endl;
        
        // 加载WAV文件
        yumo::WavInfo info;
        std::wcout << L"正在加载WAV文件..." << std::endl;
        yumo::loadWav(filename, &info);
        
        if (info.valid)
        {
            std::wcout << L"WAV文件加载成功！" << std::endl;
            std::wcout << L"原始格式:" << std::endl;
            std::wcout << L"  采样率: " << info.wf.nSamplesPerSec << L" Hz" << std::endl;
            std::wcout << L"  声道数: " << info.wf.nChannels << std::endl;
            std::wcout << L"  位深度: " << info.wf.wBitsPerSample << L" bits" << std::endl;
            std::wcout << L"  PCM数据大小: " << info.dataSize << L" bytes" << std::endl;
        }
        else
        {
            std::wcout << L"WAV文件加载失败！" << std::endl;
            return;
        }
        
        // 转换为标准格式
        std::wcout << L"\n正在转换为标准格式(44.1kHz, 双声道, 16位)..." << std::endl;
        yumo::StandardWavInfo standard = yumo::convertToStandard(info);
        std::wcout << L"转换完成！" << std::endl;
        std::wcout << L"标准格式数据大小: " << standard.size() * sizeof(int16_t) << L" bytes" << std::endl;
        
        // 播放音频
        std::wcout << L"\n按 Enter 键播放音频...";
        std::wcin.get();
        std::wcout << L"正在播放..." << std::endl;
        yumo::playStandard(standard);
        std::wcout << L"播放完成！" << std::endl;
        
    }
    catch (const yumo::exception_ex& e)
    {
        std::wcout << L"发生异常: " << e.what() << std::endl;
    }
}

int main()
{
    #ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    #endif
    
    std::wcout << L"=== Yumo Audio 重采样测试程序 ===" << std::endl;
    
    // 测试三个音频文件
    const wchar_t* files[] = {
        L"../audio/test.wav",
        L"../audio/test2.wav", 
        L"../audio/test3.wav"
    };
    
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
    {
        testAudioFile(files[i]);
    }
    
    std::wcout << L"\n=== 测试完成 ===" << std::endl;
    std::wcout << L"按 Enter 键退出...";
    std::wcin.get();
    
    return 0;
}