#as:
#source: enum.c
#source: enum-2.c
#objdump: --ctf=.ctf
#ld: -shared
#name: Conflicting Enums

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
#...
  Types:

  Strings:
#...
CTF archive member: .*enum.*\.c:
#...
  Types:
     8[0-9a-f]*: enum day_of_the_week \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x8[0-9a-f]*\) \(kind 8\) enum day_of_the_week \(aligned at 0x[0-9a-f]*\)

  Strings:
#...
CTF archive member: .*enum.*\.c:
#...
  Types:
     8[0-9a-f]*: enum day_of_the_week \(size 0x[0-9a-f]*\)
        \[0x0\] \(ID 0x8[0-9a-f]*\) \(kind 8\) enum day_of_the_week \(aligned at 0x[0-9a-f]*\)

  Strings:
#...
