#name: non-contiguous-powerpc64
#source: non-contiguous-powerpc.s
#as: -a64
#ld: -melf64ppc --enable-non-contiguous-regions -T non-contiguous-powerpc.ld
#error: .*Could not assign group.*
