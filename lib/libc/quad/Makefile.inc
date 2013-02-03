#	$NetBSD: Makefile.inc,v 1.15 2013/02/03 01:48:53 matt Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# Quad support
SRCS.quad=	cmpdi2.c ucmpdi2.c 
SRCS.quad+=	divdi3.c moddi3.c 
SRCS.quad+=	fixdfdi.c fixsfdi.c
SRCS.quad+=	fixunsdfdi.c fixunssfdi.c
SRCS.quad+=	floatdidf.c floatdisf.c
SRCS.quad+=	floatundisf.c floatundidf.c
SRCS.quad+=	muldi3.c
SRCS.quad+=	negdi2.c
SRCS.quad+=	qdivrem.c
SRCS.quad+=	udivdi3.c umoddi3.c 

.if   (${MACHINE_ARCH} == "m68k")
SRCS.quad+=	ashldi3.S ashrdi3.S lshrdi3.S
.elif (${MACHINE_ARCH} == "m68000")
SRCS.quad+=	ashldi3.S lshrdi3.S
.elif (${MACHINE_ARCH:Mearm*} != "")
SRCS.quad+=	ashrdi3.c lshldi3.c lshrdi3.c
.else
SRCS.quad+=	ashldi3.c ashrdi3.c lshrdi3.c
.endif

# XXX as far as I can tell, these are never used and can be removed
SRCS.quad+=	adddi3.c anddi3.c iordi3.c notdi2.c subdi3.c xordi3.c

SRCS+=	${SRCS.quad}

.PATH: ${ARCHDIR}/quad ${.CURDIR}/quad
