#source: pr23591a.s
#source: pr23591b.s
#source: pr23591c.s
#ld: -e _start
#readelf: -sW
#xfail: frv-*-linux* lm32-*-linux*
# frv-linux and lm32-linux fail with complaints about emitting dynamic
# relocations in read-only sections.

#...
 +[0-9]+: +[a-f0-9]+ +0 +(NOTYPE|OBJECT) +(GLOBAL +HIDDEN|LOCAL +DEFAULT) +[0-9]+ +___?start___sancov_cntrs
#pass
