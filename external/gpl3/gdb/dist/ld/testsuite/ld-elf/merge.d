#source: merge.s
#ld: -T merge.ld
#objdump: -s
#xfail: bfin-*-* cr16-*-* cris*-*-* crx-*-* d10v-*-* d30v-*-* dlx-*-*
#xfail: fr30-*-* frv-*-* ft32-*-* h8300-*-* hppa*64*-*-* ip2k-*-* iq2000-*-*
#xfail: lm32-*-* m68hc11-*-* mcore-*-* mep-*-* metag-*-* mn102*-*-* moxie-*-*
#xfail: mt-*-* nds32*-*-* nios2-*-* or32-*-* pj-*-* pru-*-* s12z-*-* score-*-*
#xfail: tic6x-*-* vax-*-* xgate-*-* xstormy16-*-* xtensa*-*-*

.*:     file format .*elf.*

Contents of section .text:
 1000 (1010)?0000(1010)? (1210)?0000(1012)? (0c)?000000(0c)? (0e)?000000(0e)? .*
Contents of section .rodata:
 1010 61626300 .*
#pass
