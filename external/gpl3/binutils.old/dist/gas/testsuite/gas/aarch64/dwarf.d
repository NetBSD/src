#readelf: -s --debug-dump=aranges
#as: -g

Symbol table '.symtab' contains 10 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    2 
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    3 
     4: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT    1 \$x
     5: 0000000000000000     0 SECTION LOCAL  DEFAULT    6 
     6: 0000000000000000     0 SECTION LOCAL  DEFAULT    8 
     7: 0000000000000000     0 SECTION LOCAL  DEFAULT    4 
     8: 0000000000000000     0 SECTION LOCAL  DEFAULT    9 
     9: 0000000000000000     8 FUNC    GLOBAL DEFAULT    1 testfunc
Contents of the .debug_aranges section:

  Length:                   44
  Version:                  2
  Offset into .debug_info:  0x0
  Pointer Size:             8
  Segment Size:             0

    Address            Length
    0000000000000000 0000000000000008 
    0000000000000000 0000000000000000 

