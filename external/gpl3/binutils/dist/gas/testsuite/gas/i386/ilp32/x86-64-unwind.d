#source: ../x86-64-unwind.s
#readelf: -S
#name: x86-64 (ILP32) unwind

There are 8 section headers, starting at offset 0x74:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] .text             PROGBITS        00000000 000034 000000 00  AX  0   0  4
  \[ 2\] .data             PROGBITS        00000000 000034 000000 00  WA  0   0  4
  \[ 3\] .bss              NOBITS          00000000 000034 000000 00  WA  0   0  4
  \[ 4\] .eh_frame         X86_64_UNWIND   00000000 000034 000008 00   A  0   0  1
  \[ 5\] .shstrtab         STRTAB          00000000 00003c 000036 00      0   0  1
  \[ 6\] .symtab           SYMTAB          00000000 0001b4 000050 10      7   5  4
  \[ 7\] .strtab           STRTAB          00000000 000204 000001 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\), l \(large\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)
#pass
