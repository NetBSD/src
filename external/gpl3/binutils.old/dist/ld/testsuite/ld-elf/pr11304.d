#source: pr11304a.s
#source: pr11304b.s
#ld: -e 0 --section-start .zzz=0x800000
#readelf: -S --wide

#failif
#...
  \[[ 0-9]+\] \.zzz[ \t]+PROGBITS[ \t0-9a-f]+AX?.*
#...
  \[[ 0-9]+\] \.zzz[ \t]+PROGBITS[ \t0-9a-f]+AX?.*
#...
