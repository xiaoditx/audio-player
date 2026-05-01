#include <windows.h>
#include <mmreg.h>
#include <stdio.h>
#include <memory>

namespace yumo
{
    /**
     * @brief WAV文件信息结构体
     *
     * 用于存储WAV文件的格式信息和音频数据
     *
     * - wf: WAVEFORMATEX结构体，包含采样率、声道数、位深度等音频格式信息
     *
     * - pcmData: 指向PCM原始数据缓冲区的指针
     *
     * - dataSize: PCM数据的大小（字节数）
     *
     * - valid: 标志位，指示是否成功加载并解析WAV文件
     */
    struct WavInfo
    {
        WAVEFORMATEX wf;                 // 音频格式信息（采样率、声道数、位深度等）
        std::unique_ptr<char[]> pcmData; // 指向PCM原始数据缓冲区的指针
        DWORD dataSize;                  // PCM数据的大小（字节数）
        bool valid = false;              // 是否成功加载并解析WAV文件
    };

    /**
     * @brief 从磁盘加载WAV文件，解析其格式块和数据块，填充WavInfo结构体
     *
     * @param[in] filename WAV文件的路径（宽字符字符串）
     * @param[out] out 指向WavInfo结构体的指针，用于存储解析后的文件信息
     * @return 无返回值，如果发生错误，将抛出异常
     *
     * @note 读取中发生任何异常，函数都将抛出异常，且out结构体的内容不保证有效。
     */
    void loadWav(const wchar_t *filename, WavInfo *out);
}