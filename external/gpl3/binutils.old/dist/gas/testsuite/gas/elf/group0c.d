#readelf: -sW
#name: group section name
#source: group0.s

#...
.*NOTYPE[ 	]+LOCAL[ 	]+DEFAULT[ 	]+[0-9]+[ 	]+\.foo_group
#pass
