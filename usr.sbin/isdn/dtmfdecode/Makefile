# $NetBSD: Makefile,v 1.2 2002/02/04 16:44:06 drochner Exp $

PROG            =       dtmfdecode
SRC             =       dtmfdecode.c

# libm is only necessary if USE_COS is defined in the source
#LDADD		+=	-lm

test:	${PROG}
	uudecode -p < ${.CURDIR}/dtmfsounds.al.uu | ./${PROG} 

.include <bsd.prog.mk>
