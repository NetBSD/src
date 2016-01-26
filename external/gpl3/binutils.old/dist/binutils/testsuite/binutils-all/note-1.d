#PROG: objcopy
#readelf: -S --wide
#objcopy: --add-section .note=$srcdir/note-1.d
#name: add notes section
#source: copytest.s
#not-target: h8300-*-*

There are .*

Section Headers:
#...
  \[[ 0-9]*\] .note             NOTE            0*0000000 0*...... 0*000... .*
#...
