#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$Id: Makefile.inc,v 1.1 1993/09/16 06:05:46 mycroft Exp $

# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c fixdfdi.c \
	fixsfdi.c fixunsdfdi.c fixunssfdi.c floatdidf.c floatdisf.c \
	floatunsdidf.c iordi3.c lshldi3.c lshrdi3.c moddi3.c muldi3.c \
	negdi2.c notdi2.c qdivrem.c subdi3.c ucmpdi2.c udivdi3.c umoddi3.c \
	xordi3.c

.PATH: ${.CURDIR}/${MACHINE}/quad ${.CURDIR}/quad
