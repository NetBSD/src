# Check that a type outside a cycle pointing into a cycle
# in another TU does not cause the whole cycle to show up
# as conflicted.  (Here, we do that by forcing conflicts
# in the variable section alone, so that we can assert that
# the type section of any conflicted dicts is empty.)
# Minimized from libbfd itself.
#as:
#source: cross-tu-cyclic-3.c
#source: cross-tu-cyclic-4.c
#objdump: --ctf=.ctf
#ld: -shared --ctf-variables
#name: cross-TU-into-cycle

.*:     file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
#...

  Labels:

  Data objects:

  Function objects:

  Variables:
    a ->  .*
    conflicty ->  .*

  Types:
     [0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct A .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* foo .*
     [0-9a-f]*: struct B .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct B .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* next .*
     [0-9a-f]*: struct B \* .*
        \[0x0\] .*
     [0-9a-f]*: struct A \* .*
        \[0x0\] .*

  Strings:
#...

CTF archive member: .*/ld/testsuite/ld-ctf/cross-tu-cyclic-[34].c:

  Header:
#...
  Labels:

  Data objects:

  Function objects:

  Variables:
    conflicty ->  .*

  Types:

  Strings:
#...
