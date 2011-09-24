# name: attributes for -march=armv6-m
# source: blank.s
# as: -march=armv6-m
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "6-M"
  Tag_CPU_arch: v6-M
  Tag_CPU_arch_profile: Microcontroller
  Tag_THUMB_ISA_use: Thumb-1
  Tag_DIV_use: Not allowed
