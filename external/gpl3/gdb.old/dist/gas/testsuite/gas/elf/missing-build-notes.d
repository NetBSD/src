# as: --generate-missing-build-notes=yes
# objdump: -r
#skip: mips*-*-openbsd

# Check that the relocations are for increasing addresses...

#...
RELOCATION RECORDS FOR \[.gnu.build.attributes\]:
OFFSET[ 	]+TYPE[ 	]+VALUE 
0+014 .*[ 	]+.*
0+0(18|1c) .*[ 	]+.*
0+0(30|38) .*[ 	]+.*
0+0(34|40) .*[ 	]+.*
#pass
