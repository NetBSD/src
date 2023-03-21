# Check that cross-TU chasing to a type in a cycle works.
# In particular, there should be only one copy of 'struct A *'.
# (Not entirely reliable: only fails if the two are emitted adjacently.
# Needs a proper objdump-CTF parser.)
#as:
#source: cross-tu-2.c
#source: cross-tu-cyclic-1.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: cross-TU-cyclic-nonconflicting

.*:     file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
#...

  Labels:

  Data objects:
#...
  Function objects:

  Variables:
#...

  Types:
#...
     [0-9a-f]*: struct A \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]\) \(kind 6\) struct A \(aligned at 0x[0-9a-f]*\)
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int a:[0-9]* \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* foo \(aligned at 0x[0-9a-f]*\)
     [0-9a-f]*: long int .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int:[0-9].*
     [0-9a-f]*: struct B \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct B \(aligned at 0x[0-9a-f]*\)
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int foo:[0-9]* \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* bar \(aligned at 0x[0-9a-f]*\)
     [0-9a-f]*: struct B \* \(size 0x[0-9a-f]*\) -> [0-9a-f]*: struct B \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* \(aligned at 0x[0-9a-f]*\)
     [0-9a-f]*: struct A \* \(size 0x[0-9a-f]*\) -> [0-9a-f]*: struct A \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* \(aligned at 0x[0-9a-f]*\)
     [0-9a-f]*: int .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int:.*

  Strings:
#...
