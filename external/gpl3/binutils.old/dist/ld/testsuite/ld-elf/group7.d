#source: ../../../binutils/testsuite/binutils-all/group.s
#source: ../../../binutils/testsuite/binutils-all/group-2.s
#source: ../../../binutils/testsuite/binutils-all/group-3.s
#source: ../../../binutils/testsuite/binutils-all/group-4.s
#ld: -r
#readelf: -g --wide
#xfail: cr16-*-* crx-*-*
# cr16 and crx use non-standard scripts with memory regions, which don't play
# well with unique group sections under ld -r.

#...
COMDAT group section \[[ 0-9]+\] `\.group' \[foo_group\] contains 2 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.foo
   \[[ 0-9]+\]   .data.foo
#...
COMDAT group section \[[ 0-9]+\] `.group' \[.text.foo\] contains 2 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.foo
   \[[ 0-9]+\]   .data.bar
#...
COMDAT group section \[[ 0-9]+\] `.group' \[foo3\] contains 2 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.foo3
   \[[ 0-9]+\]   .data.bar3
#...
COMDAT group section \[[ 0-9]+\] `.group' \[foo4\] contains 2 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.foo4
   \[[ 0-9]+\]   .data.foo4
#pass
