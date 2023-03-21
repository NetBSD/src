#source: attr-merge-arch-failed-02a.s
#source: attr-merge-arch-failed-02b.s
#source: attr-merge-arch-failed-02c.s
#source: attr-merge-arch-failed-02d.s
#as: -march-attr
#ld: -r -melf32lriscv
#warning: .*mis-matched ISA version 3.0 for 'i' extension, the output version is 2.0
#warning: .*mis-matched ISA version 3.0 for 'm' extension, the output version is 2.0
#warning: .*mis-matched ISA version 3.0 for 'a' extension, the output version is 2.0
#warning: .*mis-matched ISA version 3.0 for 'zicsr' extension, the output version is 2.0
#warning: .*mis-matched ISA version 3.0 for 'xunknown' extension, the output version is 2.0
#warning: .*mis-matched ISA version 2.1 for 'i' extension, the output version is 3.0
#warning: .*mis-matched ISA version 2.2 for 'm' extension, the output version is 3.0
#warning: .*mis-matched ISA version 2.3 for 'a' extension, the output version is 3.0
#warning: .*mis-matched ISA version 2.4 for 'zicsr' extension, the output version is 3.0
#warning: .*mis-matched ISA version 2.5 for 'xunknown' extension, the output version is 3.0
#warning: .*mis-matched ISA version 4.6 for 'i' extension, the output version is 3.0
#warning: .*mis-matched ISA version 4.7 for 'm' extension, the output version is 3.0
#warning: .*mis-matched ISA version 4.8 for 'a' extension, the output version is 3.0
#warning: .*mis-matched ISA version 4.9 for 'zicsr' extension, the output version is 3.0
#warning: .*mis-matched ISA version 4.0 for 'xunknown' extension, the output version is 3.0
#readelf: -A

Attribute Section: riscv
File Attributes
  Tag_RISCV_arch: "rv32i4p6_m4p7_a4p8_zicsr4p9_xunknown4p0"
#..
