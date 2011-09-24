# name: attributes for -mfpu=arm1020t
# source: blank.s
# as: -mfpu=arm1020t
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-1
  Tag_FP_arch: VFPv1
  Tag_DIV_use: Not allowed
