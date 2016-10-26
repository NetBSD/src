#source: startv2.s
#source: funref2.s
#as: -a64
#ld: -melf64ppc --emit-stub-syms
#ld_after_inputfiles: tmpdir/funv2.so
#readelf: -rs --wide
# Check that we do the right thing with funref2.s that doesn't have
# anything to mark it as ELFv1 or ELFv2.  Since my_func address is
# taken in a read-only section we should get a plt entry, a global
# entry stub, and my_func should be undefined dynamic with non-zero
# value.

Relocation section .* contains 1 entries:
.*
.* R_PPC64_JMP_SLOT .* my_func \+ 0

Symbol table '\.dynsym' contains 5 entries:
.*
     0: .*
     1: 0+100002b8     0 FUNC    GLOBAL DEFAULT  UND my_func
#...
Symbol table '\.symtab' contains 21 entries:
#...
    16: 0+100002b8     0 FUNC    GLOBAL DEFAULT  UND my_func
#pass
