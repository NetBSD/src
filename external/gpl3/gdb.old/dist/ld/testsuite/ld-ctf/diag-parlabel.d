#as:
#source: diag-parlabel.s
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Diagnostics - Non-zero parlabel in parent

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Compilation unit name: .*A.c
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0x37 \(0x30 bytes\)
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
