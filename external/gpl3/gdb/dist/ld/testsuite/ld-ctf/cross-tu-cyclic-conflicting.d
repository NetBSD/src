# Check that types with the same names in distinct TUs show up as
# conflicting.
#as:
#source: cross-tu-cyclic-1.c
#source: cross-tu-cyclic-2.c
#objdump: --ctf=.ctf
#ld: -shared
#name: cross-TU-cyclic-conflicting

.*:     file format .*

Contents of CTF section \.ctf:

#...
  Types:
#...
     [0-9a-f]*: long int \[0x0:0x[0-9a-f]*\] \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int:[0-9]* \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)
#...
     [0-9a-f]*: struct B .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct B .*
#...
     [0-9a-f]*: int \[0x0:0x[0-9a-f]*\] \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) int:[0-9]* \(aligned at 0x[0-9a-f]*, format 0x1, offset:bits 0x0:0x[0-9a-f]*\)
#...
     [0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 9\) struct A .*
#...
     [0-9a-f]*: struct C .*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct C .*
#...

  Strings:
#...

CTF archive member: .*/ld/testsuite/ld-ctf/cross-tu-cyclic-1\.c:
#...
  Types:
     80.*[0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x80.*\) \(kind 6\) struct A .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int a:.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* foo .*

  Strings:
#...

CTF archive member: .*/ld/testsuite/ld-ctf/cross-tu-cyclic-2\.c:
#...
  Types:
     80.*[0-9a-f]*: struct A .*
        \[0x0\] \(ID 0x80.*\) \(kind 6\) struct A .*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 1\) long int a:.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* foo .*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct C \* bar .*

  Strings:
#...
