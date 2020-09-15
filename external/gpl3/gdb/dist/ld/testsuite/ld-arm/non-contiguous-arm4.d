#name: non-contiguous-arm4
#source: non-contiguous-arm.s
#ld: --enable-non-contiguous-regions -T non-contiguous-arm4.ld
# error: \A.*Output section '.ramu' not large enough for the linker-created stubs section '.code.3.__stub'.*\Z
