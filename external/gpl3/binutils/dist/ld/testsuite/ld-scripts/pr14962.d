#ld: -Ttext=0x1000 -Tdata=0x2000 -T pr14962.t
#source: pr14962a.s
#source: pr14962b.s
#nm: -n
#notarget: rx-*-* mmix-knuth-mmixware
# The reference to x would normally generate a cross-reference error
# but the linker script converts x to absolute to avoid the error.

#...
0+2000 A x
#pass
