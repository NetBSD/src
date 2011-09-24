#source: protected3.s
#as: --64 -march=l1om
#ld: -shared -melf_l1om
#readelf: -h

ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 \(current\)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN \(Shared object file\)
  Machine:                           Intel L1OM
  Version:                           0x1
#pass
