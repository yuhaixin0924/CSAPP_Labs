# Bomb Lab 自动调试与 GDB TUI 使用指南

## 1. TUI 不需要单独安装

TUI 是 GDB 自带的终端界面。当前 `csapp-env` 容器中的 GDB 已确认支持：

```gdb
tui enable
```

TUI 可以同时显示：

- 当前汇编指令；
- 通用寄存器；
- GDB 命令窗口；
- 当前指令和断点位置；
- 单步后发生变化的寄存器。

## 2. 第一次准备

以下命令在 **macOS 宿主机终端**执行：

```bash
cd /Users/Administrator/Desktop/CSAPP_Labs/bomb
chmod +x debug.sh
```

本仓库已经将脚本设置为可执行，通常不需要再次运行 `chmod`。

## 3. 每次启动调试

### 终端 1：启动 Bomb

```bash
cd /Users/Administrator/Desktop/CSAPP_Labs/bomb
./debug.sh bomb
```

脚本会自动：

1. 检查 Docker；
2. 启动 `csapp-env`（如果尚未运行）；
3. 进入 `/workspace/bomb`；
4. 设置 Rosetta 调试端口 `1234`；
5. 如果 `answers.txt` 非空，自动使用它启动 Bomb。

刚启动时没有输出是正常现象，此时 Bomb 正在等待 GDB 连接。

### 终端 2：连接 GDB 并打开 TUI

例如分析第二关：

```bash
cd /Users/Administrator/Desktop/CSAPP_Labs/bomb
./debug.sh gdb phase_2
```

脚本会自动：

1. 启动 GDB；
2. 选择 x86-64 架构；
3. 使用 Intel 汇编语法；
4. 连接 `localhost:1234`；
5. 在 `explode_bomb` 设置断点；
6. 在指定的 `phase_2` 设置断点；
7. 打开汇编和寄存器 TUI。

Rosetta 会在 Bomb 的代码尚未映射进内存时接受 GDB 连接，此时软件、硬件
断点都可能失败。`./debug.sh bomb` 会通过命名管道暂时扣住 `answers.txt`，
让程序完成加载后等待 GDB。连接后输入：

```gdb
continue
```

终端 1 显示等待提示后，回到 GDB 终端按 `Ctrl-C`，然后输入：

```gdb
bomb-arm
continue
```

`bomb-arm` 设置断点的同时会自动放行 `answers.txt`，无需在终端1重复输入。
目标阶段断点命中后，控制权回到终端2。
如果只是使用答案文件正常运行而不调试，执行 `./debug.sh run`。

其他阶段：

```bash
./debug.sh gdb phase_1
./debug.sh gdb phase_3
./debug.sh gdb phase_4
./debug.sh gdb phase_5
./debug.sh gdb phase_6
./debug.sh gdb secret_phase
```

不需要记忆任何十六进制地址。脚本会从 Bomb 的符号表自动查询地址：

```bash
./debug.sh functions phase        # 查找名称包含 phase 的函数
./debug.sh addr phase_2           # 查看 phase_2 的真实起止地址
./debug.sh asm phase_2            # 直接查看 phase_2 汇编
./debug.sh asm read_six_numbers   # 查看辅助函数汇编
./debug.sh gdb phase_2            # 自动查地址、打断点并显示汇编
```

`asm` 使用 `objdump --prefix-addresses`，因此每条指令都会同时显示绝对地址
和相对于函数入口的偏移，例如：

```text
00000000004010f4 <phase_6>        push r14
00000000004010f6 <phase_6+0x2>    push r13
00000000004010f8 <phase_6+0x4>    push r12
```

这样可以直接把 GDB 中的 `break *phase_6+0x34` 与静态汇编对应起来。

由于 Rosetta 会错误重定位函数符号，在 GDB 内不要使用
`disassemble phase_2`；直接退出 GDB后使用 `./debug.sh asm phase_2`，
或依靠 `./debug.sh gdb phase_2` 启动时自动显示。

## 4. TUI 常用操作

| 操作 | 含义 |
|---|---|
| `layout asm` | 显示汇编窗口 |
| `layout regs` | 显示汇编和寄存器窗口 |
| `tui reg general` | 只显示通用寄存器 |
| `tui focus cmd` | 将键盘焦点交给命令窗口 |
| `refresh` | 刷新花屏界面 |
| `Ctrl+L` | 快速刷新界面 |
| `Ctrl+X`，再按 `A` | 开启或关闭 TUI |

GDB 调试命令仍然在底部的 `(gdb)` 窗口输入：

```gdb
continue
stepi
nexti
break *0x400f17
x/i $rip
x/8gx $rsp
info registers
```

### TUI 中各区域的含义

```text
上方寄存器窗口：寄存器当前值，变化的值会高亮
中间汇编窗口：当前函数附近的汇编，当前指令会被标记
下方命令窗口：输入 continue、stepi、x/i 等 GDB 命令
```

## 5. 清理卡住的旧会话

如果端口冲突、GDB 连到旧 Bomb，或者容器里残留多组进程：

```bash
./debug.sh reset
```

这会重启容器并结束其中的旧进程，不会删除 `/workspace` 中的项目文件。

然后重新执行：

```text
终端 1：./debug.sh bomb
终端 2：./debug.sh gdb phase_N
```

## 6. 查看状态

```bash
./debug.sh status
```

它会显示容器是否运行，以及当前是否存在 Bomb/GDB 进程。

## 7. 查看帮助

```bash
./debug.sh help
```

## 8. 最短记忆流程

```text
终端 1：./debug.sh bomb
终端 2：./debug.sh gdb phase_2
GDB：continue
终端 1：输入答案
终端 2：命中断点后分析
```
