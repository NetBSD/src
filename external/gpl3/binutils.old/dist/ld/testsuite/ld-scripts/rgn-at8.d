#source: rgn-at6.s
#ld: -T rgn-at8.t
#objdump: -h --wide
#xfail: rx-*-*
# Test that lma is aligned when lma_region!=region and requested by script.

#...
.* 0+10000 +0+20000 .*
.* 0+10100 +0+20100 .*
