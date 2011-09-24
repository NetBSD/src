# name: attributes for -march=armv3m
# source: blank.s
# as: -march=armv3m
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "3M"
  Tag_CPU_arch: v4
  Tag_ARM_ISA_use: Yes
  Tag_DIV_use: Not allowed
