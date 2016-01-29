#source: fill_0.s
#source: fill_1.s
#source: fill_2.s
#ld: -T fill.t
#objdump: -s -j .text
#xfail: ia64-*-* alpha-*-*ecoff m32c-*-* mips*-*-* sh-*-pe sparc*-*-coff
#xfail: tic30-*-coff tic4x-*-* tic54x-*-*
#xfail: x86_64-*-pe* x86_64-*-mingw* x86_64-*-cygwin z8k-*-*
# Breaks on ia64 due to minimum alignment of code.  The section alignment
# could be increased to suit ia64 but then we'd break many coff targets
# that don't support alignment other than 4 bytes.
# alpha-linuxecoff always aligns code to 16 bytes.
# m32c pads out code sections to 8 bytes.
# mips aligns to 16 bytes
# sh-pe pads out code sections to 16 bytes
# sparc-coff aligns to 8 bytes
# tic30-coff aligns to 2 bytes
# tic4x has 4 octet bytes
# tic54x doesn't support .p2align
# x86_64-pe aligns to 16 bytes
# z8k-coff aligns to 2 bytes

.*:     file format .*

Contents of section .text:
 [0-9a-f]+ cafebabe 01010101 02020202 12232323 .*
 [0-9a-f]+ 03030303 00345600 00004567 000089ab .*
 [0-9a-f]+ (deadbeef|efbeadde) 00004567 000089ab 0000cdef .*
 [0-9a-f]+ 00004567 000089ab 0000cdef 00000123 .*
