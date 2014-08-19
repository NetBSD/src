#PROG: readelf
#readelf: -wl
#name: MIPS DWARF-2 location information with branch swapping
#as: -32
#source: loc-swap.s

# Verify that DWARF-2 location information for instructions reordered
# into a branch delay slot is updated to point to the branch instead.

Raw dump of debug contents of section \.debug_line:

  Offset:                      0x0
  Length:                      67
  DWARF Version:               2
  Prologue Length:             33
  Minimum Instruction Length:  1
  Initial value of 'is_stmt':  1
  Line Base:                   -5
  Line Range:                  14
  Opcode Base:                 13

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
  Opcode 10 has 0 args
  Opcode 11 has 0 args
  Opcode 12 has 1 args

 The Directory Table is empty\.

 The File Name Table:
  Entry	Dir	Time	Size	Name
  1	0	0	0	loc-swap\.s

 Line Number Statements:
  Extended opcode 2: set Address to 0x0
  Special opcode 11: advance Address by 0 to 0x0 and Line by 6 to 7
  Special opcode 63: advance Address by 4 to 0x4 and Line by 2 to 9
  Special opcode 120: advance Address by 8 to 0xc and Line by 3 to 12
  Special opcode 7: advance Address by 0 to 0xc and Line by 2 to 14
  Special opcode 120: advance Address by 8 to 0x14 and Line by 3 to 17
  Special opcode 7: advance Address by 0 to 0x14 and Line by 2 to 19
  Special opcode 120: advance Address by 8 to 0x1c and Line by 3 to 22
  Special opcode 63: advance Address by 4 to 0x20 and Line by 2 to 24
  Special opcode 120: advance Address by 8 to 0x28 and Line by 3 to 27
  Special opcode 63: advance Address by 4 to 0x2c and Line by 2 to 29
  Special opcode 120: advance Address by 8 to 0x34 and Line by 3 to 32
  Special opcode 63: advance Address by 4 to 0x38 and Line by 2 to 34
  Special opcode 120: advance Address by 8 to 0x40 and Line by 3 to 37
  Special opcode 7: advance Address by 0 to 0x40 and Line by 2 to 39
  Special opcode 120: advance Address by 8 to 0x48 and Line by 3 to 42
  Special opcode 63: advance Address by 4 to 0x4c and Line by 2 to 44
  Advance PC by 24 to 0x64
  Extended opcode 1: End of Sequence
