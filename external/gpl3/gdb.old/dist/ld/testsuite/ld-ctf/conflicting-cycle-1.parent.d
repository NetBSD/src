#as:
#source: cycle-1.c
#source: A.c
#source: B.c
#source: B-2.c
#source: C.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Conflicting cycle 1.parent

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0x17 \(0x18 bytes\)
    Type section:	0x18 -- 0xc3 \(0xac bytes\)
    String section:	.*

  Labels:

  Data objects:
#...
  Function objects:

  Variables:
#...
  Types:
#...
     [0-9a-f]*: struct B \(.*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 9\) struct B \(.*
#...
CTF archive member: .*:
#...
CTF archive member: .*:
#...
