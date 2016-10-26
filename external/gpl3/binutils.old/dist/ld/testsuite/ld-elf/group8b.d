#source: group8.s
#ld: -r --gc-sections --entry bar
#readelf: -g --wide
#notarget: arc-*-* d30v-*-* dlx-*-* i960-*-* pj*-*-*
#notarget: hppa64-*-* i370-*-* i860-*-* ia64-*-* mep-*-* mn10200-*-*
#xfail: cr16-*-* crx-*-*
# generic linker targets don't support --gc-sections, nor do a bunch of others
# cr16 and crx use non-standard scripts with memory regions, which don't play
# well with unique group sections under ld -r.

COMDAT group section \[[ 0-9]+\] `.group' \[bar\] contains 1 sections:
   \[Index\]    Name
   \[[ 0-9]+\]   .text.bar
