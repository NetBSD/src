#readelf: -r -wl
#name: MIPS16 DWARF2
#as: -march=mips3 -mabi=n32 -mips16 -no-mdebug -g0
#source: mips16-dwarf2.s

Relocation section '\.rela\.debug_info' at offset .* contains 4 entries:
 *Offset * Info * Type * Sym\.Value * Sym\. Name \+ Addend
0+0006 * 0+..02 * R_MIPS_32 * 0+0000 * \.debug_abbrev \+ 0
0+000c * 0+..02 * R_MIPS_32 * 0+0000 * \.debug_line \+ 0
0+0010 * 0+..02 * R_MIPS_32 * 0+0000 * \.text \+ 0
0+0014 * 0+..02 * R_MIPS_32 * 0+0000 * \.text \+ 910

Relocation section '\.rela\.debug_line' at offset .* contains 1 entries:
 *Offset * Info * Type * Sym\.Value * Sym\. Name \+ Addend
0+0030 * 0+..02 * R_MIPS_32 * 0+0000 * .text \+ 1

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
  Extended opcode 2: set Address to 0x0
  Special opcode 5: advance Address by 0 to 0x0 and Line by 0 to 1
  Special opcode 34: advance Address by 2 to 0x2 and Line by 1 to 2
  Special opcode 34: advance Address by 2 to 0x4 and Line by 1 to 3
  Special opcode 62: advance Address by 4 to 0x8 and Line by 1 to 4
  Special opcode 34: advance Address by 2 to 0xa and Line by 1 to 5
  Special opcode 62: advance Address by 4 to 0xe and Line by 1 to 6
  Special opcode 62: advance Address by 4 to 0x12 and Line by 1 to 7
  Advance PC by 2286 to 900
  Special opcode 6: advance Address by 0 to 0x900 and Line by 1 to 8
  Advance PC by 15 to 90f
  Extended opcode 1: End of Sequence
