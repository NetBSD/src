# $NetBSD: Makefile,v 1.3 2004/03/19 08:07:22 jmc Exp $

PROG            =       dtmfdecode
SRC             =       dtmfdecode.c

# libm is only necessary if USE_COS is defined in the source
#LDADD		+=	-lm

UUDECODE_FILES	=	dtmfsounds.al

test:	${PROG} ${UUDECODE_FILES}
	./${PROG} < dtmfsounds.al

.include <bsd.files.mk>
.include <bsd.prog.mk>
