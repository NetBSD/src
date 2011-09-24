There are 9 section headers, starting at offset 0x80:

Section Headers:
  \[Nr\] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            0000000000000000 000000 000000 00      0   0  0
  \[ 1\] .text             PROGBITS        0000000000000000 000040 000000 00  AX  0   0  4
  \[ 2\] .foo              PROGBITS        0000000000000000 000040 000003 00 AXl  0   0  1
  \[ 3\] .data             PROGBITS        0000000000000000 000044 000000 00  WA  0   0  4
  \[ 4\] .bss              NOBITS          0000000000000000 000044 000000 00  WA  0   0  4
  \[ 5\] .foo.0            PROGBITS        0000000000000003 000044 000003 00 AXl  0   0  1
  \[ 6\] .shstrtab         STRTAB          0000000000000000 000047 000038 00      0   0  1
  \[ 7\] .symtab           SYMTAB          0000000000000000 0002c0 0000d8 18      8   6  8
  \[ 8\] .strtab           STRTAB          0000000000000000 000398 000016 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\), l \(large\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)
