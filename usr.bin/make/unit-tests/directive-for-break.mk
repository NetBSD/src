# $NetBSD: directive-for-break.mk,v 1.1 2022/09/02 16:24:31 sjg Exp $
#
# Tests for .break in .for loops

I= 0
LIST= 1 2 3 4 5 6 7 8

# .break terminates the loop early
# this is usually done within a conditional
.for i in ${LIST}
.if $i == 3
I:= $i
.break
.endif
.endfor
.info I=$I

# .break outside the context of a .for loop is an error
.if $I == 0
# harmless
.break
.else
# error
.break
.endif
