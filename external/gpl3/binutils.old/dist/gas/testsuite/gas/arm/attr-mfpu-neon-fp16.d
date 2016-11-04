# name: attributes for -mfpu=neon-fp16
# source: blank.s
# as: -mfpu=neon-fp16
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi* *-*-nacl*

Attribute Section: aeabi
File Attributes
  Tag_ARM_ISA_use: Yes
  Tag_THUMB_ISA_use: Thumb-1
  Tag_FP_arch: VFPv3
  Tag_Advanced_SIMD_arch: NEONv1
  Tag_FP_HP_extension: Allowed
