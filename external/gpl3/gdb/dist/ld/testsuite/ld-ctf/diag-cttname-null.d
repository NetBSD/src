#as:
#source: diag-cttname-null.s
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Diagnostics - Null type name

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
#...
  Variables:
#...
    a ->  [0-9a-f]*: struct  \(.*
#...
  Types:
#...
     [0-9a-f]*: struct  \(.*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct  \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(.*
#...
