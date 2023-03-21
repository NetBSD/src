#as:
#source: cycle-1.c
#source: A.c
#source: B.c
#source: B-2.c
#source: C.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Conflicting cycle 1.B-1

.*: +file format .*

#...
CTF archive member: .*/B.c:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Parent name: .ctf
    Compilation unit name: .*/B.c
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0x1f \(0x18 bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:
    b ->  80000001: struct B \(size 0x[0-9]*\)

  Types:
     8[0-9a-f]*: struct B .*
        \[0x0\] \(ID 0x8[0-9a-f]*\) \(kind 6\) struct B \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct C \* c \(.*

  Strings:
#...
