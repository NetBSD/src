There are .* section headers, starting at offset .*:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00 +0 +0 +0
  \[ 1\] .text             PROGBITS        00000000 000034 000008 00 +AX +0 +0 +.
  \[ 2\] .rel.+text +REL. +0+ 0+.* 00000. 0. +. +1 +4
# MIPS targets put .rela.text here.
#...
  \[ .\] .data             PROGBITS        00000000 00003c 000004 00  WA +0 +0 +.
  \[ .\] .bss              NOBITS          00000000 000040 000000 00  WA +0 +0 +.
# MIPS targets put .reginfo and .mdebug here.
# v850 targets put .call_table_data and .call_table_text here.
#...
  \[ .\] .shstrtab         STRTAB          00000000 0+.* 0+.* 00 +0 +0 +.
  \[ .\] .symtab           SYMTAB          00000000 0+.* 0+.* 10 +.. +. +4
  \[..\] .strtab           STRTAB          00000000 0+.* 0+.* 00 +0 +0 +1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\)
  I \(info\), L \(link order\), G \(group\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

