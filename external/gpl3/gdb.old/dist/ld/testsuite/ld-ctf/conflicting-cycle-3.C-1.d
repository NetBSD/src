#as:
#source: A.c
#source: A-2.c
#source: B.c
#source: B-2.c
#source: C.c
#source: C-2.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Conflicting cycle 3.C-1

.*: +file format .*

#...
CTF archive member: .*/C.c:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Parent name: .*
    Compilation unit name: .*/C.c
#...
  Labels:

  Data objects:

  Function objects:

  Variables:
    c ->  80000001: struct C \(size 0x[0-9a-f]*\)

  Types:
     8[0-9a-f]*: struct C \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x8[0-9a-f]*\) \(kind 6\) struct C \(aligned at 0x[0-9a-f]*\)
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* a \(aligned at 0x[0-9a-f]*\)

  Strings:
    0: 
#...
