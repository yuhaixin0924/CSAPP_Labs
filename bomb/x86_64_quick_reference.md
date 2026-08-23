# Bomb Lab x86-64 操作码与操作数速查表

本表使用 **Intel 语法**。在 GDB 中可这样设置：

```gdb
set disassembly-flavor intel
```

## 1. 指令基本结构

```asm
操作码 目标操作数, 源操作数
```

例如：

```asm
mov rdi, rax
```

近似于：

```c
rdi = rax;
```

阅读口诀：

```text
先看操作码做什么
再看右边数据从哪里来
最后看左边数据写到哪里
```

> `cmp`、`test`、`push`、`call` 等指令有自己的特殊语义，需要按对应说明理解。

## 2. 操作数类型

| 类型 | 示例 | 自然语言含义 |
|---|---|---|
| 立即数 | `0x8`、`10` | 指令中直接写出的常量 |
| 寄存器 | `rax`、`edi` | CPU 内部的小型存储位置 |
| 内存 | `[rax]` | 读取或写入 `rax` 指向的内存 |
| 带偏移内存 | `[rbp-0x10]` | 访问地址 `rbp-16` 中的数据 |
| 数组寻址 | `[rbx+rcx*4]` | 基址加索引乘元素大小 |
| RIP 相对内存 | `[rip+offset]` | 常用于全局变量和只读数据 |
| 指令地址 | `0x400ef7` | 跳转、调用或断点的目标 |
| 符号 | `phase_1` | 函数或变量名称 |

### 有无方括号的区别

```asm
mov rax, rbx
```

表示复制 `rbx` 中的值：

```c
rax = rbx;
```

```asm
mov rax, [rbx]
```

表示把 `rbx` 当作地址，读取该地址中的内容：

```c
rax = *rbx;
```

快速记忆：

```text
rbx    = 值或地址本身
[rbx]  = rbx 指向的内存内容
```

## 3. 内存数据大小

| Intel 写法 | 大小 | 常见 C 类型 |
|---|---:|---|
| `BYTE PTR` | 1 字节 | `char` |
| `WORD PTR` | 2 字节 | `short` |
| `DWORD PTR` | 4 字节 | `int` |
| `QWORD PTR` | 8 字节 | `long`、指针 |

例子：

```asm
mov eax, DWORD PTR [rbx]
```

```c
eax = *(int *)rbx;
```

```asm
mov rax, QWORD PTR [rbx]
```

```c
rax = *(uint64_t *)rbx;
```

## 4. 寄存器速查

### 参数、返回值和控制寄存器

| 寄存器 | 优先记忆的作用 |
|---|---|
| `rdi` | 第 1 个整数或指针参数 |
| `rsi` | 第 2 个整数或指针参数 |
| `rdx` | 第 3 个整数或指针参数 |
| `rcx` | 第 4 个整数或指针参数 |
| `r8` | 第 5 个整数或指针参数 |
| `r9` | 第 6 个整数或指针参数 |
| `rax` | 函数返回值 |
| `rsp` | 栈顶地址 |
| `rbp` | 有时作为栈帧基址 |
| `rip` | 下一条将要执行的指令地址 |

### 同一寄存器的不同宽度

| 64 位 | 32 位 | 16 位 | 8 位 |
|---|---|---|---|
| `rax` | `eax` | `ax` | `al` |
| `rbx` | `ebx` | `bx` | `bl` |
| `rcx` | `ecx` | `cx` | `cl` |
| `rdx` | `edx` | `dx` | `dl` |
| `rdi` | `edi` | `di` | `dil` |
| `rsi` | `esi` | `si` | `sil` |
| `r8` | `r8d` | `r8w` | `r8b` |

写入 32 位寄存器时，对应 64 位寄存器的高 32 位自动清零。

## 5. 数据移动指令

| 指令 | 示例 | 自然语言含义 | 近似 C |
|---|---|---|---|
| `mov` | `mov rax, rbx` | 复制数据 | `rax = rbx` |
| `mov` | `mov rax, [rbx]` | 从内存读取 | `rax = *rbx` |
| `mov` | `mov [rbx], rax` | 写入内存 | `*rbx = rax` |
| `lea` | `lea rax, [rbx+8]` | 计算地址，不访问内存 | `rax = rbx + 8` |
| `movzx` | `movzx eax, BYTE PTR [rbx]` | 读取并零扩展 | `eax = (unsigned char)*rbx` |
| `movsx` | `movsx eax, BYTE PTR [rbx]` | 读取并符号扩展 | `eax = (signed char)*rbx` |
| `movsxd` | `movsxd rax, eax` | 32 位有符号数扩展到 64 位 | `rax = (long)(int)eax` |

### `mov` 与 `lea`

```asm
mov rax, [rbx+8]
```

读取地址 `rbx+8` 中保存的数据。

```asm
lea rax, [rbx+8]
```

只计算 `rbx+8`，不读取这个地址中的数据。

## 6. 算术指令

| 指令 | 示例 | 自然语言含义 | 近似 C |
|---|---|---|---|
| `add` | `add eax, 3` | 加法 | `eax += 3` |
| `sub` | `sub eax, 3` | 减法 | `eax -= 3` |
| `inc` | `inc eax` | 加 1 | `eax++` |
| `dec` | `dec eax` | 减 1 | `eax--` |
| `imul` | `imul eax, ebx` | 有符号乘法 | `eax *= ebx` |
| `imul` | `imul eax, ebx, 4` | 乘法并写入目标 | `eax = ebx * 4` |
| `neg` | `neg eax` | 取负 | `eax = -eax` |

栈空间常见操作：

```asm
sub rsp, 0x20
```

为当前函数留出 32 字节栈空间。

```asm
add rsp, 0x20
```

释放这 32 字节栈空间。

## 7. 位运算和移位

| 指令 | 示例 | 自然语言含义 | 近似 C |
|---|---|---|---|
| `and` | `and eax, 0xf` | 按位与 | `eax &= 0xf` |
| `or` | `or eax, ebx` | 按位或 | `eax \|= ebx` |
| `xor` | `xor eax, eax` | 自己异或常用于清零 | `eax = 0` |
| `not` | `not eax` | 所有位取反 | `eax = ~eax` |
| `shl` | `shl eax, 1` | 左移，常近似乘 2 | `eax <<= 1` |
| `shr` | `shr eax, 1` | 无符号右移 | `(unsigned)eax >>= 1` |
| `sar` | `sar eax, 1` | 有符号算术右移 | `eax >>= 1` |

Bomb 中常见：

```asm
and eax, 0xf
```

只保留最低 4 位，所以结果一定在 `0～15`。

## 8. 比较与测试

| 指令 | 内部动作 | 保存结果吗 | 常见用途 |
|---|---|---:|---|
| `cmp a, b` | 计算 `a-b` | 否 | 比较两个值 |
| `test a, b` | 计算 `a&b` | 否 | 测试某些位 |
| `test a, a` | 计算 `a&a` | 否 | 判断 `a` 是否为 0 |

```asm
cmp eax, ebx
je equal
```

近似于：

```c
if (eax == ebx)
    goto equal;
```

```asm
test eax, eax
jne nonzero
```

近似于：

```c
if (eax != 0)
    goto nonzero;
```

## 9. 标志位

| 标志 | 含义 |
|---|---|
| `ZF` | 结果是否为零 |
| `SF` | 结果是否为负 |
| `CF` | 无符号运算是否产生进位或借位 |
| `OF` | 有符号运算是否溢出 |

关系：

```text
cmp/test/add/sub 等指令更新标志位
                 ↓
je/jne/jg/jl 等跳转指令读取标志位
```

## 10. 跳转指令

### 相等与不相等

| 指令 | 条件 | 自然语言 |
|---|---|---|
| `je` / `jz` | `ZF=1` | 相等或结果为零时跳转 |
| `jne` / `jnz` | `ZF=0` | 不相等或结果非零时跳转 |

### 有符号整数比较

| 指令 | 自然语言 | 典型 C 条件 |
|---|---|---|
| `jg` | 大于 | `a > b` |
| `jge` | 大于等于 | `a >= b` |
| `jl` | 小于 | `a < b` |
| `jle` | 小于等于 | `a <= b` |

### 无符号整数比较

| 指令 | 自然语言 | 典型 C 条件 |
|---|---|---|
| `ja` | 无符号大于 | `(unsigned)a > b` |
| `jae` | 无符号大于等于 | `(unsigned)a >= b` |
| `jb` | 无符号小于 | `(unsigned)a < b` |
| `jbe` | 无符号小于等于 | `(unsigned)a <= b` |

### 无条件跳转

```asm
jmp 0x400ef7
```

近似于：

```c
goto address_400ef7;
```

### Bomb 常见组合

```asm
cmp eax, 6
jne explode
```

```c
if (eax != 6)
    explode_bomb();
```

```asm
cmp eax, 6
je safe
call explode_bomb
safe:
```

```c
if (eax == 6)
    goto safe;

explode_bomb();
```

## 11. 函数和栈指令

| 指令 | 自然语言含义 | 近似动作 |
|---|---|---|
| `push rbx` | 把 `rbx` 保存到栈顶 | `rsp-=8; [rsp]=rbx` |
| `pop rbx` | 从栈顶恢复 `rbx` | `rbx=[rsp]; rsp+=8` |
| `call func` | 保存返回地址并跳到函数 | `rsp-=8; [rsp]=返回地址; rip=func` |
| `ret` | 取回返回地址并跳回 | `rip=[rsp]; rsp+=8` |
| `leave` | 撤销以 `rbp` 建立的栈帧 | 近似 `rsp=rbp; pop rbp` |

函数调用前重点检查：

```text
rdi、rsi、rdx、rcx、r8、r9
```

函数返回后重点检查：

```text
rax
```

## 12. 内存寻址公式

完整形式：

```text
[基址 + 索引×比例 + 偏移]
```

比例通常为 `1、2、4、8`。

| 示例 | 地址计算 | 常见含义 |
|---|---|---|
| `[rax]` | `rax` | 指针解引用 |
| `[rax+8]` | `rax+8` | 结构体字段或下一个指针 |
| `[rbx+rcx*4]` | `rbx+rcx×4` | `int` 数组第 `rcx` 项 |
| `[rbx+rcx*8]` | `rbx+rcx×8` | 指针或 `long` 数组第 `rcx` 项 |
| `[rbp-0x10]` | `rbp-16` | 栈上的局部变量 |
| `[rsp+0x8]` | `rsp+8` | 栈顶上方的数据 |
| `[rip+offset]` | 当前代码附近 | 全局变量或只读数据 |

数组示例：

```asm
mov eax, DWORD PTR [rbx+rcx*4]
```

如果 `rbx` 是 `int` 数组首地址：

```c
eax = array[rcx];
```

## 13. 常见控制流模式

### `if`

```asm
cmp eax, ebx
jne else_part
; 相等分支
jmp end

else_part:
; 不相等分支

end:
```

### `while`

```asm
loop_start:
cmp eax, 6
jge loop_end
; 循环体
add eax, 1
jmp loop_start

loop_end:
```

### 检查函数返回值

```asm
call some_function
test eax, eax
jne failure
```

```c
if (some_function() != 0)
    goto failure;
```

### Bomb 安全分支

```asm
cmp eax, expected
je safe
call explode_bomb

safe:
```

分析时从 `call explode_bomb` 向前寻找比较和跳转条件。

## 14. GDB 操作数查看速查

### 查看寄存器

```gdb
info registers rax rdi rsi rdx rcx rsp rbp rip
p/x $rax
p/d $eax
```

### 查看内存

```gdb
x/s $rdi
x/i $rip
x/8gx $rsp
x/6dw $rsp
x/16bx 0x402400
```

`x` 命令格式：

```text
x/数量格式单位 地址
```

显示格式：

| 格式 | 含义 |
|---|---|
| `x` | 十六进制 |
| `d` | 有符号十进制 |
| `u` | 无符号十进制 |
| `c` | 字符 |
| `s` | 字符串 |
| `i` | 汇编指令 |

数据单位：

| 单位 | 大小 |
|---|---:|
| `b` | 1 字节 |
| `h` | 2 字节 |
| `w` | 4 字节 |
| `g` | 8 字节 |

```gdb
x/8gx $rsp
```

表示从 `rsp` 指向的地址开始，查看 8 个八字节数据，以十六进制显示。

```gdb
x/i $rip
```

表示把 `rip` 指向的内存解释为下一条汇编指令。

### 单步前后对比

```gdb
x/i $rip
info registers rax rdi rsi rsp rip
x/8gx $rsp
stepi
x/i $rip
info registers rax rdi rsi rsp rip
x/8gx $rsp
```

## 15. 一分钟阅读流程

1. 圈出所有 `call`，确定调用了哪些函数。
2. 在每个 `call` 前记录 `rdi/rsi/rdx/rcx/r8/r9`。
3. 在每个 `call` 后记录 `rax` 的用途。
4. 圈出所有 `cmp`、`test` 和条件跳转。
5. 找到所有 `call explode_bomb`，向前推导爆炸条件。
6. 看到 `[]` 时写出实际地址公式。
7. 看到 `rsp` 时画出栈顶附近的数据。
8. 把每个基本块翻译成简短伪 C。

## 16. 极简记忆卡

```text
Intel 语法：目标在左，来源在右

mov   复制
lea   计算地址
add   加
sub   减
imul  乘
and   按位与
xor   按位异或
shl   左移
shr   无符号右移
sar   有符号右移

cmp   做减法，只设置标志
test  做按位与，只设置标志
je    ZF=1 时跳转
jne   ZF=0 时跳转
jmp   无条件跳转

push  压栈
pop   出栈
call  压入返回地址并跳到函数
ret   弹出返回地址并跳回

rdi rsi rdx rcx r8 r9：前六个参数
rax：返回值
rsp：栈顶
rip：下一条指令

[]：访问内存
无 []：使用值本身
```

