#source: attr-merge-div-0.s
#source: attr-merge-div-2.s
#as:
#ld: -r
#readelf: -A
# This test is only valid on ELF based ports.
# not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

Attribute Section: aeabi
File Attributes
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-1
  Tag_DIV_use: Allowed in v7-A with integer division extension
