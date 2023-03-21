#as:
#source: slice.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Slice

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Compilation unit name: .*slice.c
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0xa3 \(0x9c bytes\)
    String section:	.*
#...
  Variables:
    slices ->  [0-9a-f]*: struct slices \(size 0x4\)

  Types:
#...
     [0-9a-f]*: struct slices \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct slices \(aligned at 0x1\)
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int  one:1 \(aligned at 0x1, format 0x1, offset:bits 0x0:0x1\)
            \[0x1\] \(ID 0x[0-9a-f]*\) \(kind 1\) int  two:2 \(aligned at 0x1, format 0x1, offset:bits 0x1:0x2\)
            \[0x3\] \(ID 0x[0-9a-f]*\) \(kind 1\) int  six:6 \(aligned at 0x1, format 0x1, offset:bits 0x3:0x6\)
            \[0x9\] \(ID 0x[0-9a-f]*\) \(kind 1\) int  ten:10 \(aligned at 0x2, format 0x1, offset:bits 0x9:0xa\)
#...
