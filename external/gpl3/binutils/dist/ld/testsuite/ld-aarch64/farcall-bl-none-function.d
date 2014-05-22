#name: aarch64-farcall-bl-none-function
#source: farcall-bl-none-function.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x8001000
#error: .*\(.text\+0x0\): relocation truncated to fit: R_AARCH64_CALL26 against symbol `bar'.*
