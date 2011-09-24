#PROG: elfedit
#elfedit: --output-mach l1om
#source: empty.s
#readelf: -h
#name: Update ELF header 1
#target: x86_64-*-*

#...
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 \(current\)
#...
  Machine:                           Intel L1OM
#...
