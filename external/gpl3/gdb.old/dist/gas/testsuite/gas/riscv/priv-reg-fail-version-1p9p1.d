#as: -march=rv32if -mcsr-check -mpriv-spec=1.9.1 -march-attr
#source: priv-reg.s
#warning_output: priv-reg-fail-version-1p9p1.l
#readelf: -A

Attribute Section: riscv
File Attributes
  Tag_RISCV_arch: [a-zA-Z0-9_\"].*
  Tag_RISCV_priv_spec: 1
  Tag_RISCV_priv_spec_minor: 9
  Tag_RISCV_priv_spec_revision: 1
#...
