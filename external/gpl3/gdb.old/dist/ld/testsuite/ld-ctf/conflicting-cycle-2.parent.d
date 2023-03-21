#as:
#source: cycle-1.c
#source: A.c
#source: A-2.c
#source: B.c
#source: B-2.c
#source: C.c
#source: C-2.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Conflicting cycle 2.parent

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0x9b \(0x94 bytes\)
    String section:	0x9c -- 0xac \(0x11 bytes\)

  Labels:

  Data objects:

  Function objects:

  Variables:
    cycle_1 ->  [0-9a-f]*: struct cycle_1 \* \(size 0x[0-9a-f]*\) -> [0-9a-f]*: struct cycle_1 \(size 0x[0-9a-f]*\)

  Types:
#...
     [0-9a-f]*: struct cycle_1 \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct cycle_1 \(aligned at 0x[0-9a-f]*\)
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* a \(aligned at 0x[0-9a-f]*\)
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(aligned at 0x[0-9a-f]*\)
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct cycle_1 \* next \(aligned at 0x[0-9a-f]*\)
#...
