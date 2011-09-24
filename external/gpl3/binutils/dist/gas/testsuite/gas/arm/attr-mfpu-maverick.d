# name: attributes for -mfpu=maverick
# source: blank.s
# as: -mfpu=maverick
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-1
  Tag_DIV_use: Not allowed
