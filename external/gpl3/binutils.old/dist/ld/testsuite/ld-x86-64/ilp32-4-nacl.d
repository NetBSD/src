#source: ilp32-4.s
#as: --x32
#ld: -m elf32_x86_64_nacl -shared --no-ld-generated-unwind-info
#readelf: -d -S --wide
#target: x86_64-*-nacl*

There are 9 section headers, starting at offset 0x[0-9a-f]+:

Section Headers:
 +\[Nr\] Name +Type +Addr +Off +Size +ES +Flg +Lk +Inf +Al
 +\[ 0\] +NULL +0+ 0+ 0+ 00 +0 +0 +0
 +\[ 1\] \.text +PROGBITS +0+ 0+10000 +0+1 +00 +AX +0 +0 +1
 +\[ 2\] \.hash +HASH +100000b4 +0+b4 +0+2c +04 +A +3 +0 +4
 +\[ 3\] \.dynsym +DYNSYM +100000e0 +0+e0 +0+60 +10 +A +4 +2 +4
 +\[ 4\] \.dynstr +STRTAB +10000140 +0+140 +0+19 +00 +A +0 +0 +1
 +\[ 5\] \.dynamic +DYNAMIC +1001015c +0+15c +0+58 +08 +WA +4 +0 +4
 +\[ 6\] \.shstrtab +STRTAB +0+ +[0-9a-f]+ +0+40 +00 +0 +0 +1
 +\[ 7\] \.symtab +SYMTAB +0+0 +[0-9a-f]+ +[0-9a-f]+ +10 +8 +[0-9] +4
 +\[ 8\] \.strtab +STRTAB +0+ +[0-9a-f]+ +[0-9a-f]+ +00 +0 +0 +1
Key to Flags:
  W \(write\), A \(alloc\), X \(execute\), M \(merge\), S \(strings\), l \(large\)
  I \(info\), L \(link order\), G \(group\), T \(TLS\), E \(exclude\), x \(unknown\)
  O \(extra OS processing required\) o \(OS specific\), p \(processor specific\)

Dynamic section at offset 0x15c contains 6 entries:
  Tag        Type                         Name/Value
 0x00000004 \(HASH\)                       0x100000b4
 0x00000005 \(STRTAB\)                     0x10000140
 0x00000006 \(SYMTAB\)                     0x100000e0
 0x0000000a \(STRSZ\)                      25 \(bytes\)
 0x0000000b \(SYMENT\)                     16 \(bytes\)
 0x00000000 \(NULL\)                       0x0
