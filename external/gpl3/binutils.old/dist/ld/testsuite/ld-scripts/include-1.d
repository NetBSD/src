# name: include-1
# source: include.s
# ld: -T include-1.t
# objdump: -w -h
# xfail: "*-aix*"
# AIX targets rename the .text and .data sectons...

.*:     file format .*

Sections:
Idx +Name +Size +VMA +LMA +File off +Algn +Flags
  0 .text         0+0000020  0+0000000  0+0000000  [0-9a-f]+  2\*\*[0-9]+  CONTENTS, ALLOC, LOAD,.*CODE
  1 .data         0+0000010  0+0100000  0+0100000  [0-9a-f]+  2\*\*[0-9]+  CONTENTS, ALLOC, LOAD, DATA
