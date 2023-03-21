#as:
#source: typedef-int.c
#source: typedef-long.c
#objdump: --ctf=.ctf
#ld: -shared
#name: Conflicting Typedefs

.*: +file format .*

Contents of CTF section .ctf:

  Header:
    Magic number: dff2
    Version: 4 \(CTF_VERSION_3\)
#...
  Types:
     1: .*int .*
        .*
     [0-9]:.*int .*
        .*
     [0-9]: word .*
        \[0x0\] \(ID 0x[0-9]\) \(kind 10\) word \(aligned at 0x[48]\)

  Strings:
#...
CTF archive member: .*typedef.*\.c:
#...
  Types:
     80000001: word .*
        \[0x0\] \(ID 0x80000001\) \(kind 10\) word \(aligned at 0x[48]\)

  Strings:
#...
