#source: attr-merge-arch-failed-01a.s
#source: attr-merge-arch-failed-01b.s
#as: -march-attr
#ld: -r -melf32lriscv
#warning: .*mis-matched ISA version 3.0 for 'a' extension, the output version is 2.0
#readelf: -A

Attribute Section: riscv
File Attributes
  Tag_RISCV_arch: ".*a3p0.*"
#..
