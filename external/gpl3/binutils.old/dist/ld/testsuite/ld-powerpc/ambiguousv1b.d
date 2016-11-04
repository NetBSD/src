#source: startv1.s
#source: funref2.s
#as: -a64
#ld: -melf64ppc --emit-stub-syms
#ld_after_inputfiles: tmpdir/funv1.so
#readelf: -rs --wide
# Check that we do the right thing with funref2.s that doesn't have
# anything to mark it as ELFv1 or ELFv2.  Since my_func address is
# taken in a read-only section we should get a copy reloc for the OPD
# entry.

Relocation section .* contains 1 entries:
.*
.* R_PPC64_COPY .* my_func \+ 0

Symbol table '\.dynsym' contains 5 entries:
.*
     0: .*
     1: 0+10010408     4 FUNC    GLOBAL DEFAULT   12 my_func
#...
Symbol table '\.symtab' contains 20 entries:
#...
    15: 0+10010408     4 FUNC    GLOBAL DEFAULT   12 my_func
#pass
