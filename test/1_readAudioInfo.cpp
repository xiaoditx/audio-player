#include <iostream>
#include <io.h>
#include <fcntl.h>

int main()
{
    #ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
    #endif

    std::wcout << L"=== WAV 文件信息读取测试 ===" << std::endl;
    std::wcout << L"此测试已不可用：loadWav 接口已移至内部使用，不再对外暴露。" << std::endl;
    std::wcout << L"请使用 AudioPool::preloadAudio 或 addAudio 接口替代。" << std::endl;
    
    std::wcin.get();
    return 0;
}