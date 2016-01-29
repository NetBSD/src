#source: startv1.s
#source: funref.s
#as: -a64
#ld: -melf64ppc --emit-stub-syms
#ld_after_inputfiles: tmpdir/funv1.so
#readelf: -rs --wide
# Check that we do the right thing with funref.s that doesn't have
# anything to mark it as ELFv1 or ELFv2.  We should get a dynamic
# reloc on the function address, and my_func should be undefined
# dynamic with value zero.

Relocation section .* contains 1 entries:
.*
.* R_PPC64_ADDR64 +0+ my_func \+ 0

Symbol table '\.dynsym' contains 5 entries:
.*
     0: .*
     1: 0+00000000     0 FUNC    GLOBAL DEFAULT  UND my_func
#...
Symbol table '\.symtab' contains 19 entries:
#...
    14: 0+00000000     0 FUNC    GLOBAL DEFAULT  UND my_func
#pass
