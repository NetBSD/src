# Check that noncyclic cross-TU matching-up works.
# We can guarantee the order of emitted structures, too.
#as:
#source: cross-tu-1.c
#source: cross-tu-2.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: cross-TU-noncyclic

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Variable section:	0x0 -- 0x7 \(0x8 bytes\)
    Type section:	0x8 -- 0x7b \(0x74 bytes\)
    String section:	.*

  Labels:

  Data objects:
#...
  Function objects:

  Variables:
#...

  Types:
#...
     [0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct A .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int a:[0-9]* .*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* foo .*
#...
     [0-9a-f]*: struct B .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct B .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int foo:[0-9]* .*
#...
     [0-9a-f]*: struct B \* \(size 0x[0-9a-f]*\) -\> [0-9a-f]*: struct B .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* .*
#...
     [0-9a-f]*: struct A \* \(size 0x[0-9a-f]*\) -> [0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* .*
#...
