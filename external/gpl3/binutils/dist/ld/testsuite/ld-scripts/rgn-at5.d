# name: rgn-at5
# source: rgn-at5.s
# ld: -T rgn-at5.t -z max-page-size=0x1000
# objdump: -w -h
# target: *-*-linux* *-*-gnu*
# xfail: rx-*-*
#   FAILS on the RX because the linker has to set LMA == VMA for the
#   Renesas loader.

.*:     file format .*

Sections:
Idx +Name +Size +VMA +LMA +File off +Algn +Flags
  0 .sec0 +0+4 +0+2000 +0+2000 +0+1000 +.*
  1 .sec1 +0+4 +0+1000 +0+2004 +0+2000 +.*
  2 .sec2 +0+4 +0+4000 +0+603c +0+4000 +.*
  3 .sec3 +0+4 +0+5000 +0+5000 +0+3000 +.*
  4 .sec4 +0+4 +0+2008 +0+2008 +0+2008 +.*
  5 .sec5 +0+4 +0+200c +0+200c +0+200c +.*
#pass
