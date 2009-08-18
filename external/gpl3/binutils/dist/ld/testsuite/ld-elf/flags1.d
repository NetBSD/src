#name: --set-section-flags test 1 (sections)
#ld: -Tflags1.ld
#objcopy_linked_file: --set-section-flags .post_text_reserve=contents,alloc,load,readonly,code 
#readelf: -l --wide
#xfail: "arm*-*-*" "xscale-*-*"
#xfail: "avr-*-*" "dlx-*-*" "h8300-*-*" "m32r-*-*" "msp430-*-*"
#xfail: "*-*-hpux*"
# Fails on the ARM because the .section type character is % rather than @.
# Fails on the AVR, DLX, H8300, M32R and MSP430 because the two sections
#  are not merged into one segment.  (There is no good reason why they have to be).
# Fails on HPUX systems because the .type pseudo-op behaves differently.

#...
Program Headers:
  Type.*
  LOAD +0x[0-9a-f]+ 0x0*0 0x0*0 0x0*01(6[1-9a-f]|70) 0x0*01(6[1-9a-f]|70) RWE 0x[0-9a-f]+

#...
  Segment Sections...
   00[ \t]+.text .post_text_reserve[ \t]*
#pass
