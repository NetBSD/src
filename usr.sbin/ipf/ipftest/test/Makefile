#	$NetBSD: Makefile,v 1.6 2000/06/12 13:35:51 veego Exp $
#
# (C)opyright 1993-1996 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
.include <bsd.prog.mk>

tests: first ftests ptests ntests

first:
	-mkdir -p results

# Filtering tests
ftests: f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f14

# Rule parsing tests
ptests: i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11

ntests: n1 n2 n3 n4 n5 n6 n7

0:
	@(cd ..; make ipftest; )

f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f14:
	@/bin/sh ${.CURDIR}/dotest $@ ${.CURDIR}/../../../../dist/ipf/test

f12:
	@/bin/sh ${.CURDIR}/hextest $@ ${.CURDIR}/../../../../dist/ipf/test

i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11:
	@/bin/sh ${.CURDIR}/itest $@ ${.CURDIR}/../../../../dist/ipf/test

n1 n2 n3 n4 n5 n6 n7:
	@/bin/sh ${.CURDIR}/nattest $@ ${.CURDIR}/../../../../dist/ipf/test

clean cleandir:
	/bin/rm -f f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f14
	/bin/rm -f i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11
	/bin/rm -f n1 n2 n3 n4 n5 n6 n7
	/bin/rm -rf results
