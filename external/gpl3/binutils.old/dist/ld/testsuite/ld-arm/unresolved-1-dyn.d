#source: unresolved-1.s
#ld: --warn-unresolved tmpdir/mixed-lib.so
#warning: \(\.text\+0x8\): warning: undefined reference to `foo'
#readelf: -r

Relocation section '\.rel\.dyn' .*
 Offset .*
.* R_ARM_GLOB_DAT +00000000 +foo
