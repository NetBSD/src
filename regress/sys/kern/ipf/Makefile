#	$NetBSD: Makefile,v 1.2 2002/05/13 06:34:14 martti Exp $
#
# (C)opyright 1993-1996 by Darren Reed.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and due credit is given
# to the original author and the contributors.
#
.include <bsd.prog.mk>

regress tests: first ftests ptests ntests nitests logtests intests

first:
	-mkdir -p results

# Filtering tests
ftests: f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f12 f13 f14 f15 f16 f17

# Rule parsing tests
ptests: i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11

ntests: n1 n2 n3 n4 n5 n6 n7

nitests: ni1 ni2 ni4 ni5

intests: in1 in2 in3 in4

logtests: l1

ipv6: ipv6.1 ipv6.2

0:
	@(cd ..; make ipftest; )

f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f14:
	@/bin/sh ${.CURDIR}/dotest $@ ${.CURDIR}

f12 f13:
	@/bin/sh ${.CURDIR}/hextest $@ ${.CURDIR}

f15 f16:
	@/bin/sh ${.CURDIR}/mtest $@ ${.CURDIR}

f17:
	@/bin/sh ${.CURDIR}/mhtest $@ ${.CURDIR}

i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11:
	@/bin/sh ${.CURDIR}/itest $@ ${.CURDIR}

n1 n2 n3 n4 n5 n6 n7:
	@/bin/sh ${.CURDIR}/nattest $@ ${.CURDIR}

ni1 ni2 ni4 ni5:
	@/bin/sh ${.CURDIR}/natipftest $@ ${.CURDIR}

in1 in2 in3 in4:
	@/bin/sh ${.CURDIR}/intest $@ ${.CURDIR}

l1:
	@/bin/sh ${.CURDIR}/logtest $@ ${.CURDIR}

ipv6.1 ipv6.2:
	@/bin/sh ${.CURDIR}/dotest6 $@ ${.CURDIR}

clean:
	/bin/rm -f f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f11 f13 f12 f14 f15 f16 f17
	/bin/rm -f i1 i2 i3 i4 i5 i6 i7 i8 i9 i10 i11
	/bin/rm -f n1 n2 n3 n4 n5 n6 n7
	/bin/rm -f ni1 ni2 ni3 ni4 ni5
	/bin/rm -f in1 in2 in3 in4
	/bin/rm -f l1
	/bin/rm -f ipv6.1 ipv6.2
	/bin/rm -f results/*
