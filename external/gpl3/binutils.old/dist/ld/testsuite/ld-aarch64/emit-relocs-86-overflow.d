#source: emit-relocs-86-overflow.s
#as: -mabi=ilp32
#ld: -m [aarch64_choose_ilp32_emul] -e0 --emit-relocs
#objdump: -dr
#error: .*\(.text\+0x\d+\): relocation truncated to fit: R_AARCH64_P32_TLSLD_ADD_DTPREL_LO12 against symbol `v2' .*
