#name: PR 3041 data
#objdump: -trs

.*:     file format .*

SYMBOL TABLE:
00000006  w      \.text 0000 00 0f mytext
00000014  w      \.data 0000 00 10 mydata
00000040  w      \.bss  0000 00 11 mybss

RELOCATION RECORDS FOR \[\.data\]:
OFFSET   TYPE              VALUE 
00000004 32                mytext
00000008 32                mytext
0000000c 32                mytext
00000010 32                mydata
00000014 32                mydata
00000018 32                mydata
0000001c 32                mybss
00000020 32                mybss
00000024 32                mybss

Contents of section .text:
 0000 4e714e71 4e714e71                    .*
Contents of section .data:
 0008 12345678 00000000 00000001 fffffffd  .*
 0018 00000000 00000003 fffffffe 00000000  .*
 0028 00000002 ffffffff                    .*
