#as:
#source: cycle-1.c
#source: A.c
#source: B.c
#source: B-2.c
#source: C.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Conflicting cycle 1.B-2

.*: +file format .*

#...
CTF archive member: .*/B-2.c:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Parent name: .ctf
    Compilation unit name: .*/B-2.c
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0x2b \(0x24 bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:
    b ->  80000001: struct B \(.*

  Types:
     8[0-9a-f]*: struct B \(.*
        \[0x0\] \(ID 0x8[0-9a-f]*\) \(kind 6\) struct B \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct C \* c \(.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 1\) int wombat:32 \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)

  Strings:
#...
