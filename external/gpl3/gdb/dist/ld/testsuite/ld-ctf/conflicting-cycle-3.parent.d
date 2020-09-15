#as:
#source: A.c
#source: A-2.c
#source: B.c
#source: B-2.c
#source: C.c
#source: C-2.c
#objdump: --ctf=.ctf
#ld: -shared
#name: Conflicting cycle 3

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Type section:	0x0 -- 0x57 \(0x58 bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:

  Types:
#...
     [0-9a-f]*: int \[0x0:0x[0-9a-f]*\] \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int:[0-9]* \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)
#...
  Strings:
    0: 
#...
