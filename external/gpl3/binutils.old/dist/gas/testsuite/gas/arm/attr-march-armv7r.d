# name: attributes for -march=armv7r
# source: blank.s
# as: -march=armv7r
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi *-*-nacl*

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "7R"
  Tag_CPU_arch: v7
  Tag_CPU_arch_profile: Realtime
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-2
