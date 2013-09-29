#PROG: objcopy
#objdump: -h
#objcopy: --set-section-flags .text=alloc,data
#name: copy with setting section flags 3
#source: bintest.s
#not-target: *-*-*aout *-*-*pe *-*-*coff hppa*-*-hpux* i*86-*-cygwin* i*86-*-mingw32* m68k-*-netbsd m68k-*-openbsd* ns32k-*-netbsd x86_64-*-mingw*
# The .text # section in PE/COFF has a fixed set of flags and these
# cannot be changed.  We skip it for them.

.*: +file format .*

Sections:
Idx.*
#...
  [0-9]* .text.*
                  CONTENTS, ALLOC, LOAD, RELOC, DATA
#...
