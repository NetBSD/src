#source: reloc-005.s
#ld: -T $srcdir/$subdir/reloc-007.ld
#objdump: -D
# now that we treat addresses as wrapping, it isn't possible to fail
#error: relocation truncated to fit: R_D10V_18_PCREL

# Test 18 bit pc rel reloc bad boundary
#pass
