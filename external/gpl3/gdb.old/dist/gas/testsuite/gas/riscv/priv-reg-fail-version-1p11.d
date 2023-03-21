#as: -march=rv32if -mcsr-check -mpriv-spec=1.11 -march-attr
#source: priv-reg.s
#warning_output: priv-reg-fail-version-1p11.l
#readelf: -A

Attribute Section: riscv
File Attributes
  Tag_RISCV_arch: [a-zA-Z0-9_\"].*
  Tag_RISCV_priv_spec: 1
  Tag_RISCV_priv_spec_minor: 11
#...
