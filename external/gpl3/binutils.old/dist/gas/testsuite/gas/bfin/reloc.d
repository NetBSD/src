#objdump: -r
#name: reloc
.*: +file format .*

RELOCATION RECORDS FOR \[\.text\]:
OFFSET   TYPE              VALUE 
0*0004 R_BFIN_PCREL24    _call_data1
0*0008 R_BFIN_RIMM16     .data
0*000a R_BFIN_PCREL12_JUMP_S  .text\+0x00000018
0*000e R_BFIN_PCREL24    call_data1\+0x00000008
0*0012 R_BFIN_HUIMM16    .data\+0x00000002
0*0016 R_BFIN_LUIMM16    .data\+0x00000004
0*001a R_BFIN_HUIMM16    load_extern1


RELOCATION RECORDS FOR \[\.data\]:
OFFSET   TYPE              VALUE 
0*0006 R_BFIN_BYTE_DATA  load_extern1


