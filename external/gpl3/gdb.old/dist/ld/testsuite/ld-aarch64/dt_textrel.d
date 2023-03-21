#source: dt_textrel.s
#target: [check_shared_lib_support]
#ld: -shared
#readelf: -d
#...
.*TEXTREL.*
#pass
