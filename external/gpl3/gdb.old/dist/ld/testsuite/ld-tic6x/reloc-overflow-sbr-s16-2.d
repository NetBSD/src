#name: C6X relocation underflow, SBR_S16
#as: -mlittle-endian
#ld: -melf32_tic6x_le -Tsbr.ld --defsym s=0xffff807f
#source: reloc-overflow-sbr-s16.s
#error: .*relocation truncated to fit: R_C6000_SBR_S16.*
