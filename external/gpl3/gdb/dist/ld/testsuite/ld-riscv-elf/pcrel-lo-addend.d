#name: %pcrel_lo section symbol with an addend
#source: pcrel-lo-addend.s
#as: -march=rv32ic
#ld: -melf32lriscv
#error: .*dangerous relocation: %pcrel_lo section symbol with an addend
