# 注：本测试脚本基本由AI完成，部分参数尚未修改，不能使用

import os
import time
import subprocess
import sys

# ---------- 配置区 ----------
# 待检测的编译器列表
test_list = ["g++", "i686-w64-mingw32-g++"]

# 编译选项（例如警告、优化）
compile_options = ["-Wall", "-O2"]
# 链接的库（可根据需要修改）
link_libs = ["-lstdc++", "-lm", "-lwinmm"]

src_files = ["../src/audioPlayer.cpp"]

# 当前目录下的测试文件命名规则（支持 编号_名字.cpp），我们列出所有 .cpp
# ---------- 辅助函数 ----------
def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def get_cpp_files():
    """返回当前目录下所有 .cpp 文件，按名称排序"""
    files = [f for f in os.listdir('.') if f.endswith('.cpp')]
    files.sort()
    return files

def check_compiler(compiler_name):
    """检测某个编译器是否可用（通过 --version）"""
    try:
        subprocess.run([compiler_name, "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    except FileNotFoundError:
        return False

def compile_file(source, compiler, bits):
    """编译单个文件，bits 为 32 或 64，返回是否成功"""
    output_name = source.replace('.cpp', '')
    if bits == 32:
        output_name += "_32.exe"
        # 32位使用 i686-w64-mingw32-g++，如果调用时传入的 compiler 已经是交叉编译器，就不用额外参数
        # 但如果用的是普通 g++ 想生成32位需要加 -m32，这里约定 compiler 决定位数，不再加参数
    else:
        output_name += "_64.exe"
    
    cmd = [compiler] + compile_options + [source, "-o", output_name] + link_libs
    print(f"执行命令: {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if result.returncode == 0:
        print(f"✅ 编译成功：{output_name}")
        return True
    else:
        print(f"❌ 编译失败：{source}")
        return False

# ---------- 三个主要功能 ----------
def environmentTest():
    clear_screen()
    print("=== 环境检测 ===")
    for compiler in test_list:
        if check_compiler(compiler):
            print(f"[√] {compiler} 已安装")
        else:
            print(f"[×] {compiler} 未安装或不在 PATH")
    print("\n按回车键返回主菜单...")
    input()

def startCompile():
    clear_screen()
    files = get_cpp_files()
    if not files:
        print("当前目录下没有找到 .cpp 文件！")
        time.sleep(1)
        return
    
    print("=== 选择要编译的文件 ===")
    for idx, fname in enumerate(files, start=1):
        print(f"{idx}. {fname}")
    
    choice = input("\n输入编号（单个，如 3）或区间（如 2-5），或 'all' 全部编译：").strip()
    
    # 解析用户选择，得到要编译的文件列表
    selected = []
    if choice.lower() == 'all':
        selected = files
    elif '-' in choice:
        try:
            start, end = map(int, choice.split('-'))
            if 1 <= start <= len(files) and 1 <= end <= len(files) and start <= end:
                selected = files[start-1:end]
            else:
                print("区间超出范围！")
                time.sleep(1)
                return
        except:
            print("区间格式错误，请使用如 2-5")
            time.sleep(1)
            return
    else:
        try:
            idx = int(choice)
            if 1 <= idx <= len(files):
                selected = [files[idx-1]]
            else:
                print("编号超出范围！")
                time.sleep(1)
                return
        except:
            print("非法输入！")
            time.sleep(1)
            return
    
    if not selected:
        return
    
    # 选择 32 / 64 位
    print("\n编译为 32 位还是 64 位？")
    bits_choice = input("输入 32 或 64：").strip()
    if bits_choice not in ('32', '64'):
        print("无效选择，取消编译")
        return
    
    # 根据选择的位数决定编译器
    if bits_choice == '32':
        compiler = "i686-w64-mingw32-g++"
        if not check_compiler(compiler):
            print(f"错误：{compiler} 未安装，无法编译32位程序")
            return
    else:
        compiler = "g++"
        if not check_compiler(compiler):
            print(f"错误：{compiler} 未安装，无法编译64位程序")
            return
    
    print(f"\n开始编译 {len(selected)} 个文件（{bits_choice}位）...\n")
    for src in selected:
        compile_file(src, compiler, int(bits_choice))
        print()
    
    print("编译完成。按回车键返回...")
    input()

def buildAll():
    clear_screen()
    print("=== 一键构建全部 ===")
    
    # 检测可用编译器
    have_gcc = check_compiler("g++")
    have_mingw32 = check_compiler("i686-w64-mingw32-g++")
    
    files = get_cpp_files()
    if not files:
        print("当前目录下没有 .cpp 文件，无法构建")
        time.sleep(1)
        return
    
    print(f"发现 {len(files)} 个源文件")
    
    # 根据已有环境决定构建方式
    if have_gcc and have_mingw32:
        print("检测到 g++ (64位) 和 i686-w64-mingw32-g++ (32位)，将同时编译 64位 和 32位 版本。")
        build_64 = True
        build_32 = True
    elif have_gcc:
        print("仅检测到 g++，将只编译 64位 版本。")
        build_64 = True
        build_32 = False
    elif have_mingw32:
        print("仅检测到 i686-w64-mingw32-g++，将只编译 32位 版本。")
        build_64 = False
        build_32 = True
    else:
        print("未检测到任何可用编译器，无法构建。")
        time.sleep(2)
        return
    
    # 开始编译
    for src in files:
        print(f"\n处理 {src}：")
        if build_64:
            print("  -> 编译 64位...")
            compile_file(src, "g++", 64)
        if build_32:
            print("  -> 编译 32位...")
            compile_file(src, "i686-w64-mingw32-g++", 32)
    
    print("\n一键构建完成。按回车键返回...")
    input()

# ---------- 主菜单循环（已由你提供） ----------
# 注意：你的主循环在代码下方，需要把上面的函数定义放在主循环之前。
# 我已经把三个函数定义好了，请把下面的内容放在脚本末尾。

while True:
    print("""欢迎使用yumo库测试集快速编译脚本
    （本版本不可用）
    1. 测试环境检测
    2. 开始编译
    3. 一键开始
    """)

    choice = input("输入你的选择：")

    if choice == "1":
        environmentTest()
    elif choice == "2":
        startCompile()
    elif choice == "3":
        buildAll()
    else:
        print("非法输入！")
        time.sleep(1)
        clear_screen()