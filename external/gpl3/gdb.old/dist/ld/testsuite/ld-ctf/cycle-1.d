#as:
#source: cycle-1.c
#source: A.c
#source: B.c
#source: C.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Cycle 1

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0x1f \(0x20 bytes\)
    Type section:	0x20 -- 0xc7 \(0xa8 bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:
#...
  Types:
#...
     [0-9a-f]*: struct cycle_1 \(.*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct cycle_1 \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* a \(.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct cycle_1 \* next \(.*
#...
