# Bomb Lab x86-64 汇编对照手册

这份手册针对当前 Bomb Lab 的 `main` 反汇编结果，统一使用 **Intel 语法**。

## 1. 如何阅读一行反汇编

```asm
0x0000000000400e37 <+151>: mov rdi, rax
```

| 部分 | 含义 |
|---|---|
| `0x400e37` | 这条指令在内存中的地址 |
| `<+151>` | 距离 `main` 函数入口 151 字节 |
| `mov` | 操作码（指令名） |
| `rdi` | 目标操作数 |
| `rax` | 源操作数 |

Intel 语法通常是：

```text
指令 目标, 来源
```

所以：

```asm
mov rdi, rax
```

近似于：

```c
rdi = rax;
```

`mov` 是复制，执行后 `rax` 的值仍然保留。

## 2. 常用寄存器

| 64 位寄存器 | 低 32 位 | 常见作用 |
|---|---|---|
| `rax` | `eax` | 函数返回值、临时数据 |
| `rbx` | `ebx` | 保存中间数据；本程序中保存 `argv` |
| `rdi` | `edi` | 第一个函数参数 |
| `rsi` | `esi` | 第二个函数参数 |
| `rdx` | `edx` | 第三个函数参数 |
| `rcx` | `ecx` | 第四个函数参数 |
| `r8` | `r8d` | 第五个函数参数 |
| `r9` | `r9d` | 第六个函数参数 |
| `rsp` | `esp` | 栈顶地址 |
| `rip` | `eip` | 下一条将要执行的指令地址 |

Linux x86-64 的整数和指针参数通常按以下顺序传递：

```text
第 1 个参数 → rdi
第 2 个参数 → rsi
第 3 个参数 → rdx
第 4 个参数 → rcx
第 5 个参数 → r8
第 6 个参数 → r9
返回值      → rax
```

进入 `main(int argc, char **argv)` 时：

```text
edi = argc
rsi = argv
```

### 64 位和 32 位名称

```text
rax：完整 64 位
eax：rax 的低 32 位
```

其他寄存器同理，例如 `rdi/edi`、`rsi/esi`。

整数经常使用 32 位寄存器，指针和地址通常使用 64 位寄存器。写入 `edi` 等 32 位寄存器时，对应 64 位寄存器的高 32 位会被清零。

## 3. 操作数类型

### 3.1 立即数

直接写在指令中的数字：

```asm
cmp edi, 0x1
mov edi, 0x8
```

`0x` 表示十六进制：

```text
0x1 = 1
0x8 = 8
```

### 3.2 寄存器操作数

```asm
mov rbx, rsi
```

含义：

```c
rbx = rsi;
```

### 3.3 内存操作数

方括号 `[]` 表示访问内存：

```asm
mov rdi, QWORD PTR [rsi+0x8]
```

含义不是把 `rsi+8` 复制到 `rdi`，而是读取地址 `rsi+8` 中保存的内容：

```c
rdi = *(uint64_t *)(rsi + 8);
```

可以暂时把方括号理解为 C 语言中的解引用 `*`：

```text
[地址] ≈ *(地址)
```

### 3.4 数据大小

| 写法 | 读取或写入的大小 |
|---|---:|
| `BYTE PTR` | 1 字节 |
| `WORD PTR` | 2 字节 |
| `DWORD PTR` | 4 字节 |
| `QWORD PTR` | 8 字节 |

x86-64 中一个指针占 8 字节，所以读取指针时经常使用 `QWORD PTR`。

## 4. 常见指令

### 4.1 `mov`：复制数据

```asm
mov 目标, 来源
```

示例：

```asm
mov rbx, rsi
```

```c
rbx = rsi;
```

从内存读取：

```asm
mov rdi, QWORD PTR [rsi+0x8]
```

```c
rdi = *(uint64_t *)(rsi + 8);
```

写入内存：

```asm
mov QWORD PTR [rip+0x2029b4], rax
```

```c
*(uint64_t *)(rip + 0x2029b4) = rax;
```

### 4.2 `cmp`：比较两个值

```asm
cmp 左边, 右边
```

CPU 内部近似计算：

```text
左边 - 右边
```

结果不会保存，只会更新 `ZF`、`SF`、`CF`、`OF` 等标志位。

```asm
cmp edi, 0x1
jne somewhere
```

整体近似于：

```c
if (edi != 1)
    goto somewhere;
```

### 4.3 `test`：测试位或判断是否为零

```asm
test 左边, 右边
```

CPU 内部进行按位与，但不保存结果，只更新标志位：

```text
左边 & 右边
```

常见写法：

```asm
test rax, rax
jne somewhere
```

自己与自己后，结果是否为零就等于原值是否为零，因此整体近似于：

```c
if (rax != 0)
    goto somewhere;
```

### 4.4 条件跳转

跳转指令不直接比较寄存器，而是读取之前由 `cmp`、`test` 等指令设置的标志位。

| 指令 | 含义 |
|---|---|
| `je` | `ZF=1` 时跳转，常表示相等或结果为零 |
| `jne` | `ZF=0` 时跳转，常表示不相等或结果非零 |
| `jg` | 有符号大于时跳转 |
| `jge` | 有符号大于等于时跳转 |
| `jl` | 有符号小于时跳转 |
| `jle` | 有符号小于等于时跳转 |
| `ja` | 无符号大于时跳转 |
| `jb` | 无符号小于时跳转 |
| `jmp` | 不检查条件，直接跳转 |

例如：

```asm
test eax, eax
je safe
call explode_bomb
```

近似于：

```c
if (eax == 0)
    goto safe;

explode_bomb();
```

### 4.5 `call`：调用函数

```asm
call read_line
```

`call` 会保存返回地址并跳到被调用函数。函数完成后，通常通过 `ret` 返回，返回值通常放在 `rax`。

```asm
call read_line
mov  rdi, rax
call phase_1
```

近似于：

```c
char *input = read_line();
phase_1(input);
```

### 4.6 `push` 和 `pop`：保存与恢复栈数据

```asm
push rbx
```

把当前 `rbx` 保存到栈顶。

```asm
pop rbx
```

从栈顶取回原值并放回 `rbx`。

函数开头和结尾经常成对出现：

```asm
push rbx
; 使用 rbx
pop  rbx
ret
```

### 4.7 `ret`：函数返回

```asm
ret
```

从栈中取得返回地址，回到调用该函数的位置。

## 5. 标志位与跳转原理

CPU 中有一组状态标志：

| 标志位 | 常见含义 |
|---|---|
| `ZF` | 结果是否为零 |
| `SF` | 结果是否为负 |
| `CF` | 是否产生无符号进位或借位 |
| `OF` | 是否发生有符号溢出 |

例如：

```asm
cmp eax, ebx
je equal
```

`cmp` 计算 `eax-ebx`。如果两者相等，结果是 0，于是 `ZF=1`；`je` 看到 `ZF=1` 后跳转。

```asm
test eax, eax
je zero
```

如果 `eax` 是 0，`eax & eax` 的结果也是 0，于是 `ZF=1`，`je` 跳转。

## 6. 指令地址与跳转目标

```asm
400ef0: je   400ef7
400ef2: call explode_bomb
400ef7: add  rsp, 0x8
400efb: ret
```

每行左边是该指令在内存中的地址。`je 400ef7` 表示条件满足时，把 `rip` 改为 `0x400ef7`。

两条执行路线：

```text
ZF=1：0x400ef0 → 0x400ef7，跳过 explode_bomb
ZF=0：0x400ef0 → 0x400ef2，调用 explode_bomb
```

用 GDB 查看地址对应的指令：

```gdb
x/i 0x400ef7
x/4i 0x400ef0
x/i $rip
```

`rip` 保存下一条将要执行的指令地址。

## 7. `rip` 相对寻址

```asm
mov rax, QWORD PTR [rip+0x20299b]
```

表示以当前代码位置为基准，加上偏移量后访问内存。GDB 会在右侧给出计算后的地址和符号：

```asm
mov rax,QWORD PTR [rip+0x20299b]  # 0x603748 <stdin>
```

可理解为：

```c
rax = stdin;
```

如果下一条是：

```asm
mov QWORD PTR [rip+0x2029b4],rax  # 0x603768 <infile>
```

两条合起来就是：

```c
infile = stdin;
```

`#` 后面是反汇编工具给出的注释，不是 CPU 执行的指令。

## 8. `argv` 的内存结构

进入 `main` 时：

```text
rsi = argv
```

在 64 位系统中，每个指针占 8 字节：

```text
rsi      → argv[0] 的指针
rsi + 8  → argv[1] 的指针
rsi + 16 → argv[2] 的指针
```

因此：

```asm
mov rdx, QWORD PTR [rsi]
```

近似于：

```c
rdx = argv[0];
```

而：

```asm
mov rdi, QWORD PTR [rsi+0x8]
```

近似于：

```c
rdi = argv[1];
```

注意：

```text
rsi     = argv 数组的地址
[rsi]   = argv[0]
[rsi+8] = argv[1]
```

## 9. `@plt` 的含义

```asm
call fopen@plt
call puts@plt
call exit@plt
```

`@plt` 表示程序通过 PLT 跳转表调用动态链接库中的函数。初学时可以直接这样理解：

```text
fopen@plt → fopen
puts@plt  → puts
exit@plt  → exit
```

## 10. `main` 的主要逻辑

### 10.1 没有文件参数

```asm
cmp edi, 0x1
jne 0x400db6
```

`edi` 是 `argc`。如果 `argc==1`，程序把 `stdin` 保存到 `infile`：

```c
if (argc == 1) {
    infile = stdin;
}
```

对应运行方式：

```bash
./bomb
```

Bomb 从标准输入读取答案。

### 10.2 有一个答案文件参数

如果 `argc==2`，程序执行的逻辑近似为：

```c
infile = fopen(argv[1], "r");

if (infile == NULL) {
    printf("无法打开文件");
    exit(8);
}
```

对应运行方式：

```bash
./bomb answers.txt
```

### 10.3 六个阶段的重复结构

```asm
call read_line
mov  rdi, rax
call phase_1
call phase_defused
```

数据流：

```text
read_line 读取一行
        ↓
rax = 字符串地址
        ↓
mov rdi, rax
        ↓
rdi = phase_1 的第一个参数
        ↓
call phase_1
```

近似 C 代码：

```c
char *input = read_line();
phase_1(input);
phase_defused();
```

Phase 2 到 Phase 6 使用相同结构。

### 10.4 整体伪 C

```c
int main(int argc, char **argv)
{
    if (argc == 1) {
        infile = stdin;
    }
    else if (argc == 2) {
        infile = fopen(argv[1], "r");
        if (infile == NULL)
            exit(8);
    }
    else {
        exit(8);
    }

    initialize_bomb();

    phase_1(read_line());
    phase_defused();

    phase_2(read_line());
    phase_defused();

    phase_3(read_line());
    phase_defused();

    phase_4(read_line());
    phase_defused();

    phase_5(read_line());
    phase_defused();

    phase_6(read_line());
    phase_defused();

    return 0;
}
```

## 11. GDB 查看命令

### 查看函数和指令

```gdb
info functions
info functions phase
disassemble main
disassemble phase_1
disassemble /r phase_1
x/10i $rip
x/i 0x400ef7
```

### 查看寄存器

```gdb
info registers
info registers rax rdi rsi rsp rip
p/x $rax
p/d $eax
```

### 查看内存

```gdb
x/s 0x402400
x/s $rdi
x/16bx 0x402400
x/6dw $rsp
x/8gx $rsp
```

| 命令 | 含义 |
|---|---|
| `x/s 地址` | 按字符串查看 |
| `x/i 地址` | 按汇编指令查看 |
| `x/10i 地址` | 查看 10 条指令 |
| `x/16bx 地址` | 查看 16 个单字节十六进制数 |
| `x/6dw 地址` | 查看 6 个四字节十进制整数 |
| `x/8gx 地址` | 查看 8 个八字节十六进制数 |

内存本身没有“字符串”或“指令”的标签，`/s`、`/i` 等格式决定 GDB 如何解释相同的字节。例如，对字符串地址使用 `x/i` 会得到没有意义的伪指令。

### 断点和运行

```gdb
break phase_1
break explode_bomb
info breakpoints
continue
stepi
nexti
bt
```

| 命令 | 含义 |
|---|---|
| `break phase_1` | 在第一关入口暂停 |
| `break explode_bomb` | 在真正爆炸前暂停 |
| `continue` | 继续运行到下一个断点 |
| `stepi` | 执行一条指令，会进入被调用函数 |
| `nexti` | 执行一条指令，通常不进入被调用函数 |
| `bt` | 查看函数调用链 |

## 12. Bomb 分析的固定流程

1. 在当前阶段和 `explode_bomb` 设置断点。
2. 让程序停在阶段入口。
3. 使用 `disassemble phase_N` 查看函数。
4. 找出所有 `call explode_bomb`。
5. 从爆炸调用向前查看 `cmp`、`test` 和跳转条件。
6. 判断怎样才能跳过爆炸。
7. 把汇编逐段翻译成伪 C。
8. 用 `x/s`、`x/d` 和寄存器命令验证数据。
9. 求出满足全部条件的输入。
10. 将正确答案保存到 `answers.txt`。

核心思路：

```text
找到 explode_bomb
        ↓
向前寻找比较与跳转
        ↓
推导避免爆炸的条件
        ↓
把条件转换为正确输入
```

## 13. 快速记忆表

```text
mov    复制
cmp    做减法但不保存，只设置标志
test   做按位与但不保存，只设置标志
je     ZF=1 时跳转
jne    ZF=0 时跳转
jmp    无条件跳转
call   调用函数
ret    函数返回
push   保存到栈
pop    从栈恢复

没有 []：通常表示值或地址本身
有 []：访问这个地址中的内容

rdi/rsi/rdx/rcx/r8/r9：前六个整数或指针参数
rax：函数返回值
rsp：栈顶地址
rip：下一条指令地址
```
