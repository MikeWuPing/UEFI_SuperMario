# SMB UEFI — 超级马力欧兄弟 UEFI Shell 移植

将红白机（NES）《超级马力欧兄弟》（Super Mario Bros）移植到 **UEFI Shell** 环境下运行。图形输出使用 UEFI GOP（Graphics Output Protocol），纯软件渲染，无音频依赖。可在 QEMU + OVMF 或真机 UEFI Shell 下运行。
<img width="1268" height="825" alt="full1" src="https://github.com/user-attachments/assets/1e26143e-acd9-4914-8f37-9c0c8fc4fd17" />

## 项目来源

本项目基于 [nukep/smb-vanilla-port](https://github.com/nukep/smb-vanilla-port) 的 C/C++ 反编译代码进行移植。原始项目将 NES 6502 汇编反编译为现代 C 代码，保留原始游戏逻辑的精确行为（包括 Bug）。
<img width="768" height="716" alt="welcomescreen" src="https://github.com/user-attachments/assets/f9b356f7-b13c-481b-858c-2cec74bc777a" />
<img width="1258" height="789" alt="into game" src="https://github.com/user-attachments/assets/a43035e4-c4b1-466a-8052-b07562792fae" />


## 改进与原项目的差异

| 特性 | 原始 smb-vanilla-port | 本 UEFI 移植 |
|------|----------------------|-------------|
| 运行平台 | SDL2/SDL3 (Windows/Linux) | UEFI Shell (QEMU/真机) |
| 图形渲染 | OpenGL / 软件光栅化 (SDL surface) | GOP 帧缓冲（双缓冲 + 整数缩放） |
| 音频 | SDL 音频（完整 APU 模拟） | 无（UEFI 不支持音频回调） |
| 输入 | SDL 键盘/手柄映射 | UEFI SimpleTextIn（定时器非阻塞轮询） |
| ROM 加载 | 外部 .nes 文件 | 编译时嵌入 PRG+CHR 二进制（无 iNES 头） |
| 构建系统 | Meson | MSVC 批处理 / Makefile (clang) |
| 文件系统 | 操作系统文件 API | 无（ROM 嵌入 EFI 二进制） |
| 调试 | 标准输出 | 串口 COM1 (QEMU `-serial stdio`) |
| 版权保护 | 需用户提供 .nes ROM 文件 | PRG/CHR 数据从 .nes 剥离为独立 .bin 文件 |

### 核心改进详情

1. **ROM 数据与代码分离**：从 iNES 格式 ROM 中剥离出 `prg.bin`（32KB 游戏数据表）和 `chr.bin`（8KB 图形 tile），去除有版权的 16 字节 iNES 头。两个纯数据文件编译时嵌入，代码仓库不含任何任天堂版权数据。

2. **UEFI 协议驱动**：基于最小化 UEFI 类型定义（不依赖 EDK2/gnu-efi SDK），使用 GOP 帧缓冲直接渲染、SimpleTextIn 轮询键盘、定时器事件驱动帧率。

3. **双缓冲渲染**：256×240 像素先绘制到后台缓冲区，帧结束时通过 nearest-neighbor 整数缩放到 GOP 帧缓冲，消除画面撕裂。

4. **关键 Bug 修复**：修复了 `DrawSpriteObject` 中的水平翻转实现——原反编译代码的 tile 数据索引使用了翻转后的坐标（导致像素顺序未真正镜像），现已与 NES PPU 行为一致。

## 构建与运行

### 前置条件

- [QEMU](https://www.qemu.org/)（已包含 OVMF 固件于 `share/edk2-x86_64-code.fd`）
- [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) 含 C++ 桌面开发工具

### 构建

```bat
cd uefi
build.bat
```

产物：`build/smb.efi`（同步到 `uefi/smb.efi`）

### 运行

```sh
qemu-system-x86_64 \
  -drive if=pflash,format=raw,file=/path/to/edk2-x86_64-code.fd \
  -drive file=fat:rw:uefi,format=raw \
  -m 256 -net none -serial stdio -display sdl
```

UEFI Shell 启动后自动执行 `startup.nsh` 进入游戏。

## 按键映射

| NES 按键 | 键盘 |
|---------|------|
| 方向键 | 方向键 |
| A（跳跃） | 1 或 / |
| B（加速/火球） | 2 或 . |
| Start | Enter |
| Select | Space |
| 退出 | Esc |

## 项目结构

```
uefi/
├── uefi_types.h        # UEFI 类型定义（GOP/File/Input/Boot Services）
├── uefi_compat.h/.c    # C 标准库兼容层（memset/printf/sscanf 等）
├── uefi_render.h/.c    # GOP 软件渲染器（双缓冲 + 整数缩放）
├── uefi_input.h/.c     # 键盘输入（定时器 + WaitForEvent 非阻塞）
├── uefi_rom.h/.c       # ROM 数据访问（内存嵌入 + 文件回退）
├── uefi_debug.h/.c     # 串口调试输出（COM1 0x3F8）
├── uefi_main.c         # 入口 + 游戏主循环
├── embedded_rom.c      # prg.bin + chr.bin 编译时嵌入
├── build.bat           # MSVC 构建脚本
├── Makefile            # clang 构建规则
├── startup.nsh         # UEFI Shell 自动启动脚本
├── prg.bin             # PRG-ROM 数据表（32KB）
└── chr.bin             # CHR-ROM 图案表（8KB）
```

## 许可与声明

- 本项目游戏逻辑代码继承自 [nukep/smb-vanilla-port](https://github.com/nukep/smb-vanilla-port)
- 本仓库**不包含**任天堂的版权数据（ROM 文件）。构建需要用户自行提供合法的 `smb.nes`，工具链从中提取无版权的纯数据表（PRG/CHR .bin 文件）
- UEFI 前端代码为原创实现，欢迎自由使用和修改

---

# SMB UEFI — Super Mario Bros for UEFI Shell

A port of the NES classic "Super Mario Bros" to the **UEFI Shell** environment. Uses UEFI GOP for graphics, pure software rendering, no audio. Runs on QEMU + OVMF or bare-metal UEFI systems.
<img width="1268" height="825" alt="full4" src="https://github.com/user-attachments/assets/61f4d615-11ce-401a-8cf6-3c48e5b7a10d" />
## Origin

Based on the C/C++ decompilation from [nukep/smb-vanilla-port](https://github.com/nukep/smb-vanilla-port), which reverse-engineered the NES 6502 assembly into modern C code while preserving exact game behavior (bugs included).
<img width="768" height="716" alt="welcomescreen" src="https://github.com/user-attachments/assets/f9b356f7-b13c-481b-858c-2cec74bc777a" />
<img width="1258" height="789" alt="into game" src="https://github.com/user-attachments/assets/a43035e4-c4b1-466a-8052-b07562792fae" />
## Key Improvements

- **IP-safe ROM handling**: PRG-ROM and CHR-ROM extracted as bare `.bin` files without the copyrighted iNES header. No Nintendo data in this repository.
- **UEFI-native**: Zero-dependency UEFI type definitions — no EDK2 or gnu-efi SDK required
- **Double buffering**: Eliminates screen tearing via back-buffer → GOP framebuffer flush with integer nearest-neighbor scaling
- **Bug fix**: Corrected horizontal sprite flipping in the rendering pipeline (tile data now indexed by original coordinates, matching NES PPU behavior)
- **Serial debug**: COM1 port output for QEMU `-serial stdio` debugging

## Build & Run

```bat
cd uefi
build.bat          # MSVC build → build/smb.efi
```

```sh
qemu-system-x86_64 -drive if=pflash,format=raw,file=OVMF.fd \
  -drive file=fat:rw:uefi,format=raw -m 256 -serial stdio -display sdl
```

## License

Game logic derived from nukep/smb-vanilla-port. UEFI frontend is original. No copyrighted ROM data is included in this repository.
