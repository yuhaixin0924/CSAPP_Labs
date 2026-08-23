# Bomb Lab 的 GDB 自动初始化配置
# 本文件只负责连接和显示，不会自动执行 continue。

set architecture i386:x86-64
set disassembly-flavor intel
set pagination off

target remote localhost:1234

# Rosetta 在 ELF 尚未映射时既不能插入软件断点，也拒绝硬件断点。
# 先 continue 到 Bomb 等待第一行输入，再按 Ctrl-C；此时代码已经映射，
# 使用 bomb-arm 插入普通软件断点。
set $bomb_target = 0

define bomb-arm
  if $bomb_target == 0
    printf "没有选择目标函数；请使用 break 函数名手工设置断点。\n"
  else
    break *$bomb_target
    break explode_bomb
    shell touch /tmp/csapp-bomb-gdb-ready
    printf "断点已设置，answers.txt 已放行。现在输入 continue。\n"
  end
end
document bomb-arm
在 Bomb 完成加载后，设置目标函数和爆炸断点。
end

tui enable
layout asm
layout regs
focus cmd
