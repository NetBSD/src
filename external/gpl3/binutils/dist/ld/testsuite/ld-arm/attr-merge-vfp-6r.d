#source: attr-merge-vfpv3xd.s
#source: attr-merge-vfp-3.s
#as:
#ld: -r
#readelf: -A
# This test is only valid on ELF based ports.
# not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

Attribute Section: aeabi
File Attributes
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-1
  Tag_FP_arch: VFPv3
  Tag_ABI_HardFP_use: SP and DP
  Tag_DIV_use: Not allowed
