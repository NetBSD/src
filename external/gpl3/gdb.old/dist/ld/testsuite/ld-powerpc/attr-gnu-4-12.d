#source: attr-gnu-4-1.s
#source: attr-gnu-4-2.s
#as: -a32
#ld: -r -melf32ppc
#error: .* uses hard float, .* uses soft float
#target: powerpc*-*-*
