#as:
#source: super-sub-cycles.c
#objdump: --ctf=.ctf
#ld: -shared
#name: Super- and sub-cycles

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
    Compilation unit name: .*super-sub-cycles.c
#...
    Type section:	.*\(0x108 bytes\)
#...
  Types:
#...
     [0-9a-f]*: struct A \(.*
        \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct A \(.*
            \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct B b \(.*
                \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct C c \(.*
                    \[0x0\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct A \* a \(.*
                    \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct D d \(.*
                        \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(.*
                \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct D d \(.*
                    \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct B \* b \(.*
            \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct X x \(.*
                \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct Y y \(.*
                    \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 6\) struct Z z \(.*
                        \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct Y \* y \(.*
                        \[0x[0-9a-f]*\] \(ID 0x[0-9a-f]*\) \(kind 3\) struct D \* d \(.*
#...
