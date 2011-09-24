#source: ../../../binutils/testsuite/binutils-all/group-2.s
#ld: -r
#readelf: -Sg --wide
#xfail: cr16-*-* crx-*-* xstormy*-*-*
# cr16 and crx use non-standard scripts with memory regions, which don't play
# well with unique group sections under ld -r.
# xstormy also uses a non-standard script, putting .data before .text.

#...
  \[[ 0-9]+\] .group[ \t]+GROUP[ \t]+.*
#...
  \[[ 0-9]+\] \.text.*[ \t]+PROGBITS[ \t0-9a-f]+AXG[ \t]+.*
#...
  \[[ 0-9]+\] \.data.*[ \t]+PROGBITS[ \t0-9a-f]+WAG[ \t]+.*
#...
COMDAT group section \[[ 0-9]+\] `.group' \[.text.foo\] contains 2 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.*
   \[[ 0-9]+\]   .data.*
#pass
