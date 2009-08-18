#source: lns-big-delta.s
#readelf: -wl
#name: lns-big-delta
Raw dump of debug contents of section \.debug_line:
#...
 Line Number Statements:
  Extended opcode 2: set Address to .*
  Copy
  Advance Line by 1 to 2
  Extended opcode 2: set Address to .*
  Copy
  Advance PC by fixed size amount 0 to .*
  Extended opcode 1: End of Sequence
#pass
