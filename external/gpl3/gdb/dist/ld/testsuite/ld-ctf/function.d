#as:
#source: function.c
#objdump: --ctf=.ctf
#ld: -shared
#name: Function

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Compilation unit name: .*function.c
#...
    Type section:	0x0 -- 0x8f \(0x90 bytes\)
    String section:	.*
#...
  Types:
#...
     [0-9a-f]*: int \(\*\) \(char, int, float, void \*, void \(\*\)\(\*\) \(int\)\) \(size 0x0\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 5\) int \(\*\) \(char, int[0-9]*, float, void \*, void \(\*\)\(\*\) \(int\)\) \(aligned at 0x[0-9a-f]*\)
#...
