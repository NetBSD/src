# $NetBSD: Makefile,v 1.1.1.1 2001/01/06 13:00:36 martin Exp $

PROG            =       dtmfdecode
SRC             =       dtmfdecode.c

# libm is only necessary if USE_COS is defined in the source
#LDADD		+=	-lm

test:	${PROG}
	uudecode -p < dtmfsounds.al.uu | ./${PROG} 

.include <bsd.prog.mk>
