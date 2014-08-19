#as: --x32
#ld: -m elf32_x86_64 -shared --no-ld-generated-unwind-info
#readelf: -d -S --wide
#target: x86_64-*-linux*

There are 9 section headers, starting at offset 0x1d8:

Section Headers:
  \[Nr\] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  \[ 0\]                   NULL            00000000 000000 000000 00      0   0  0
  \[ 1\] .hash             HASH            00000094 000094 00002c 04   A  2   0  4
  \[ 2\] .dynsym           DYNSYM          000000c0 0000c0 000060 10   A  3   2  4
  \[ 3\] .dynstr           STRTAB          00000120 000120 000019 00   A  0   0  1
  \[ 4\] .text             PROGBITS        0000013c 00013c 000001 00  AX  0   0  4
  \[ 5\] .dynamic          DYNAMIC         00200140 000140 000058 08  WA  3   0  4
  \[ 6\] .shstrtab         STRTAB          00000000 000198 000040 00      0   0  1
  \[ 7\] .symtab           SYMTAB          00000000 [0-9a-f]+ [0-9a-f]+ 10      8   [0-9]  4
  \[ 8\] .strtab           STRTAB          00000000 [0-9a-f]+ [0-9a-f]+ 00      0   0  1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\), l \(large\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Dynamic section at offset 0x140 contains 6 entries:
  Tag        Type                         Name/Value
 0x00000004 \(HASH\)                       0x94
 0x00000005 \(STRTAB\)                     0x120
 0x00000006 \(SYMTAB\)                     0xc0
 0x0000000a \(STRSZ\)                      25 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x00000000 \(NULL\)                       0x0
