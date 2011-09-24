#PROG: objcopy
#objdump: -h
#objcopy: --set-section-flags foo=contents,alloc,load,code
#name: copy with setting section flags 2
#source: copytest.s
#not-target: *-*-*aout *-*-aix* h8300-*-* hppa-*-hpux* m68k-*-netbsd m68k-*-openbsd* mips-sony-bsd* mips-*-ultrix* mips-*-osf* mips-*-ecoff* mips-*-irix mips-*-irix[2-4] mips-*-riscos* ns32k-*-netbsd
# Note - we use copytest.s and a section named "foo" rather
# than .text because for some file formats (eg PE) the .text
# section has a fixed set of flags and these cannot be changed.

.*: +file format .*

Sections:
Idx.*
#...
  [0-9]* foo.*
                  CONTENTS, ALLOC, LOAD, CODE
#...
