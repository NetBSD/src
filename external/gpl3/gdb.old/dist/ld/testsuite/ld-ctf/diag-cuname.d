#as:
#source: diag-cuname.s
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: Diagnostics - Invalid CU name offset

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Compilation unit name: \(\?\)
    Variable section:	.*
    Type section:	.*
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
    [0-9a-f]*: \(\?\)
#...
