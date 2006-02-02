#readelf: -r -wl
#name: MIPS16 DWARF2
#as: -mabi=32 -mips16 -no-mdebug -g0
#source: mips16-dwarf2.s

Relocation section '\.rel\.debug_info' at offset .* contains 4 entries:
 *Offset * Info * Type * Sym\.Value * Sym\. Name
0+0006 * 0+..02 * R_MIPS_32 * 0+0000 * \.debug_abbrev
0+000c * 0+..02 * R_MIPS_32 * 0+0000 * \.debug_line
0+0010 * 0+..02 * R_MIPS_32 * 0+0000 * \.text
0+0014 * 0+..02 * R_MIPS_32 * 0+0000 * \.text

Relocation section '\.rel\.debug_line' at offset .* contains 1 entries:
 *Offset * Info * Type * Sym\.Value * Sym\. Name
0+0030 * 0+..02 * R_MIPS_32 * 0+0000 * \.text

Dump of debug contents of section \.debug_line:

  Length:                      64
  DWARF Version:               2
  Prologue Length:             35
  Minimum Instruction Length:  1
  Initial value of 'is_stmt':  1
  Line Base:                   -5
  Line Range:                  14
  Opcode Base:                 10
  \(Pointer size:               4\)

 Opcodes:
  Opcode 1 has 0 args
  Opcode 2 has 1 args
  Opcode 3 has 1 args
  Opcode 4 has 1 args
  Opcode 5 has 1 args
  Opcode 6 has 0 args
  Opcode 7 has 0 args
  Opcode 8 has 0 args
  Opcode 9 has 1 args

 The Directory Table is empty\.

 The File Name Table:
  Entry	Dir	Time	Size	Name
  1	0	0	0	mips16-dwarf2\.s

 Line Number Statements:
  Extended opcode 2: set Address to 0x1
  Special opcode 5: advance Address by 0 to 0x1 and Line by 0 to 1
  Special opcode 34: advance Address by 2 to 0x3 and Line by 1 to 2
  Special opcode 34: advance Address by 2 to 0x5 and Line by 1 to 3
  Special opcode 62: advance Address by 4 to 0x9 and Line by 1 to 4
  Special opcode 34: advance Address by 2 to 0xb and Line by 1 to 5
  Special opcode 62: advance Address by 4 to 0xf and Line by 1 to 6
  Special opcode 62: advance Address by 4 to 0x13 and Line by 1 to 7
  Advance PC by 2286 to 901
  Special opcode 6: advance Address by 0 to 0x901 and Line by 1 to 8
  Advance PC by 15 to 910
  Extended opcode 1: End of Sequence
