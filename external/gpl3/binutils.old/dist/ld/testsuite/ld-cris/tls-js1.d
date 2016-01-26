#source: dsov32-1.s
#source: tls-ld-4.s
#source: dsov32-2.s
#source: expdyn1.s
#source: tls-hx.s
#source: dso-1.s
#as: --pic --no-underscore --em=criself --march=v32
#ld: --shared -m crislinux
#readelf: -a

# DSO with a R_CRIS_16_DTPREL and a R_CRIS_32_PLT_PCREL.  The .got.plt
# byte index (a) and .rela.plt item index (b) are in sync as b=a/4-3
# *except* when there's a R_CRIS_DTPMOD, because while the relocated
# contents goes in .got.plt, the relocation goes in .rela.got, not
# .rela.plt.  And, it'd cover 8 bytes in .got.plt, not 4 bytes.
# Making sure .rela.plt has the right contents; no R_CRIS_NONE entries.

#...
  .* .got[ 	]+PROGBITS[ 	]+0+2348 0+348 0+20 04  WA  0   0  4
#...
Relocation section '\.rela\.dyn' at offset 0x20c contains 2 entries:
 Offset     Info    Type            Sym\.Value  Sym\. Name \+ Addend
00002354  0000001e R_CRIS_DTPMOD +0
00002364  0000050a R_CRIS_GLOB_DAT   00002368   expobj \+ 0

Relocation section '\.rela\.plt' at offset 0x224 contains 2 entries:
 Offset     Info    Type            Sym\.Value  Sym\. Name \+ Addend
0000235c  0000030b R_CRIS_JUMP_SLOT  00000296   dsofn4 \+ 0
00002360  00000c0b R_CRIS_JUMP_SLOT  000002ae   dsofn \+ 0

The decoding of unwind sections for machine type Axis Communications 32-bit embedded processor is not currently supported.
#pass
