# name: attributes for -march=armv4xm
# source: blank.s
# as: -march=armv4xm
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "4XM"
  Tag_CPU_arch: v4
  Tag_ARM_ISA_use: Yes
  Tag_DIV_use: Not allowed
