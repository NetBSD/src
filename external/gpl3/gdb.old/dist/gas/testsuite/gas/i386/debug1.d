#as: --gdwarf-2
#readelf: -wl
#name: DWARF2 debugging information 1

Raw dump of debug contents of section \.z?debug_line:

  Offset:                      0x0
  Length:                      .*
  DWARF Version:               3
  Prologue Length:             .*
  Minimum Instruction Length:  1
  Initial value of 'is_stmt':  1
  Line Base:                   -5
  Line Range:                  14
  Opcode Base:                 13

 Opcodes:
  Opcode 1 has 0 args
  Opcode 2 has 1 arg
  Opcode 3 has 1 arg
  Opcode 4 has 1 arg
  Opcode 5 has 1 arg
  Opcode 6 has 0 args
  Opcode 7 has 0 args
  Opcode 8 has 0 args
  Opcode 9 has 1 arg
  Opcode 10 has 0 args
  Opcode 11 has 0 args
  Opcode 12 has 1 arg

 The Directory Table \(offset 0x.*\):
  .*

 The File Name Table \(offset 0x.*\):
  Entry	Dir	Time	Size	Name
  1	1	0	0	debug1.s

 Line Number Statements:
  \[0x.*\]  Extended opcode 2: set Address to 0x0
  \[0x.*\]  Special opcode 7: advance Address by 0 to 0x0 and Line by 2 to 3
  \[0x.*\]  Special opcode 20: advance Address by 1 to 0x1 and Line by 1 to 4
  \[0x.*\]  Special opcode 20: advance Address by 1 to 0x2 and Line by 1 to 5
  \[0x.*\]  Advance PC by 1 to 0x3
  \[0x.*\]  Extended opcode 1: End of Sequence


