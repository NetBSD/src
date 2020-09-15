#as:
#source: array-char.c
#source: array-int.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Arrays

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0xf \(0x10 bytes\)
    Type section:	0x10 -- 0x6b \(0x5c bytes\)
    String section:	.*

  Labels:

  Data objects:

  Function objects:

  Variables:
    digits.*\[10] .*
    digits.*\[10] .*

  Types:
#...
     [0-9a-f]*: .*\[10\] .*
#...
     [0-9a-f]*: .*\[10\] .*
#...
