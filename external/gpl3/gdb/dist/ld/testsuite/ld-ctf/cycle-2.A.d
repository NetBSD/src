#as:
#source: A.c
#source: B.c
#source: C.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Cycle 2.A

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0x17 \(0x18 bytes\)
    Type section:	0x18 -- 0x83 \(0x6c bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:
#...
    a ->  [0-9a-f]*: struct A \(.*
#...
  Types:
#...
     [0-9a-f]*: struct A \(.*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct A \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(.*
#...
  Strings:
    0: 
#...
    [0-9a-f]*: A
#...
