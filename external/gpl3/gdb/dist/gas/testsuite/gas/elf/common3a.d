#source: common3.s
#as: --elf-stt-common=yes
#readelf: -s -W

#...
 +[0-9]+: +0+4 +30 +COMMON +GLOBAL +DEFAULT +COM +foobar
#pass
