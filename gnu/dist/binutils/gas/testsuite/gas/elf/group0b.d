#readelf: -g
#name: group section
#source: group0.s

#...
COMDAT group section `.foo_group' \[.foo_group\] contains 2 sections:
[ 	]+\[Index\][ 	]+Name
[ 	]+\[.*\][ 	]+.foo
[ 	]+\[.*\][ 	]+.bar
#pass
