# SMB UEFI 产品说明书

## 产品概述

SMB UEFI 是将任天堂红白机（NES）经典游戏《超级马力欧兄弟》（Super Mario Bros）移植到 UEFI Shell 环境的纯软件实现。无需操作系统支持，直接在 UEFI 固件层面运行，适用于 QEMU 虚拟机或有 UEFI 固件的真机设备。

## 技术规格

| 项目 | 说明 |
|------|------|
| 目标平台 | UEFI 2.70+ Shell (x86_64) |
| 图形协议 | EFI Graphics Output Protocol (GOP) |
| 渲染方式 | 软件光栅化，双缓冲，整数 nearest-neighbor 缩放 |
| 分辨率 | NES 原生 256×240，自动适配屏幕（最大 5× 整数缩放） |
| 帧率 | ~60 FPS（定时器驱动） |
| 输入 | UEFI SimpleTextIn 协议，非阻塞轮询 |
| 音频 | 不支持 |
| ROM 格式 | 编译时嵌入 PRG-ROM (32KB) + CHR-ROM (8KB)，无 iNES 头 |
| 构建工具 | Visual Studio 2019/2022 (MSVC) 或 clang + lld |
| 产物 | smb.efi (~250KB 单文件) |

## 功能特性

### 游戏功能

- 完整 SMB1 游戏逻辑（32 个关卡，8 个世界）
- 标题画面、Demo 自动演示
- 双人模式（马里奥 + 路易吉轮流）
- 所有敌人、道具、隐藏砖块、管道、藤蔓
- 分数、金币、生命计数
- 水下关卡、地下关卡、城堡关卡

### 技术特性

- **零操作系统依赖**：直接在 UEFI 固件上运行
- **双缓冲渲染**：消除画面撕裂，视觉流畅
- **整数缩放**：NES 256×240 画面自适应屏幕，保持像素锐利
- **非阻塞输入**：定时器 + WaitForEvent 机制，游戏循环不阻塞
- **串口调试**：COM1 实时输出帧计数和按键事件
- **自动启动**：startup.nsh 脚本实现上电即玩

## 系统架构

```
┌─────────────────────────────────────────────┐
│                  smb.efi                     │
├─────────────────────────────────────────────┤
│  UEFI 前端 (uefi/*.c)                        │
│  ├── uefi_main.c    入口 + 游戏主循环         │
│  ├── uefi_render.c  GOP 双缓冲渲染器          │
│  ├── uefi_input.c   键盘输入                 │
│  ├── uefi_rom.c     ROM 数据管理             │
│  └── uefi_debug.c   串口调试                 │
├─────────────────────────────────────────────┤
│  SMB 核心库 (smb/src/*.cpp + smbcore/*.c)    │
│  ├── smbcore.cpp    调度器 + ROM 加载        │
│  ├── smb1.cpp       SMB1 Reset/NMI          │
│  ├── common.c       游戏逻辑 (11732行)       │
│  ├── smb1only.c     SMB1 跳转表             │
│  └── common_sound.c 音频引擎 (静默运行)      │
├─────────────────────────────────────────────┤
│  UEFI 固件                                   │
│  ├── GOP (Graphics Output Protocol)          │
│  ├── SimpleTextIn (键盘)                     │
│  └── Boot Services (内存/定时器)             │
└─────────────────────────────────────────────┘
```

## 安装与运行

### 环境要求

- QEMU 8.0+ 或真机 UEFI x86_64 固件
- OVMF 固件文件（QEMU 自带 `share/edk2-x86_64-code.fd`）

### 构建步骤

```bat
cd uefi
build.bat
```

生成 `build/smb.efi`，自动复制到 `uefi/smb.efi`。

### 运行

```sh
qemu-system-x86_64 \
  -drive if=pflash,format=raw,file=edk2-x86_64-code.fd \
  -drive file=fat:rw:uefi,format=raw \
  -m 256 -net none -serial stdio -display sdl
```

UEFI Shell 启动后 `startup.nsh` 自动执行 `smb.efi`。

## 操作说明

| 操作 | 按键 |
|------|------|
| 移动 | ↑ ↓ ← → |
| 跳跃 | 1 或 / |
| 加速/发火球 | 2 或 . |
| 开始/暂停 | Enter |
| 选择 | Space |
| 退出游戏 | Esc |

## 文件清单

| 文件 | 大小 | 说明 |
|------|------|------|
| smb.efi | ~250KB | UEFI 可执行文件 |
| prg.bin | 32KB | 游戏数据表（关卡/物理/敌人等） |
| chr.bin | 8KB | 图形图案表（512 个 8×8 tile） |
| startup.nsh | 16B | 自动启动脚本 |

## 已知限制

1. 不支持音频（UEFI 下无音频回调机制）
2. 不支持 SMB2J（Lost Levels）——仅编译了 SMB1 模式
3. 键盘映射中字母键（Z/X/J/K）被 QEMU SDL 拦截，改用数字键和符号键
4. 真机 UEFI 未经充分测试，主要验证环境为 QEMU + OVMF
