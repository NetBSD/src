#source: common3.s
#as: --elf-stt-common=no
#readelf: -s -W

#...
 +[0-9]+: +0+4 +30 +OBJECT +GLOBAL +DEFAULT +COM +foobar
#pass
