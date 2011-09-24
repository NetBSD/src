# name: attributes for -march=armv2s
# source: blank.s
# as: -march=armv2s
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "2S"
  Tag_CPU_arch: v4
  Tag_ARM_ISA_use: Yes
  Tag_DIV_use: Not allowed
