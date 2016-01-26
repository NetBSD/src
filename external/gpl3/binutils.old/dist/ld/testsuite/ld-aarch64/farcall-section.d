#name: Aarch64 farcall to symbol of type STT_SECTION
#source: farcall-section.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x8001014
#error: .*\(.text\+0x0\): relocation truncated to fit: R_AARCH64_CALL26 against `.foo'
