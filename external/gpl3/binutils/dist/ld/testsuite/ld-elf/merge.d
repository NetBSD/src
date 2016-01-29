#source: merge.s
#ld: -T merge.ld
#objdump: -s
#xfail: "arc-*-*" "avr-*-*" "bfin-*-*" "cr16-*-*" "cris*-*-*" "crx-*-*" "d10v-*-*" "d30v-*-*"
#xfail: "dlx-*-*" "fr30-*-*" "frv-*-*" "hppa*64*-*-*" "h8300-*-*" "score-*-*"
#xfail: "i370-*-*" "i860-*-*" "i960-*-*" "ip2k-*-*" "iq2000-*-*" "lm32-*-*"
#xfail: "mcore-*-*" "mn102*-*-*" "ms1-*-*" "mep-*-*"
#xfail: "or32-*-*" "pj-*-*" "sparc*-*-*" "tic6x-*-*" "vax-*-*" "xstormy16-*-*"
#xfail: "xtensa*-*-*" "metag-*-*"

.*:     file format .*elf.*

Contents of section .text:
 1000 (1010)?0000(1010)? (1210)?0000(1012)? (0c)?000000(0c)? (0e)?000000(0e)? .*
Contents of section .rodata:
 1010 61626300 .*
#pass
