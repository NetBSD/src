#	from: @(#)Makefile	5.2 (Berkeley) 3/5/91
#	$Id: Makefile,v 1.1 1993/09/01 21:14:13 jtc Exp $
#

.PATH:	${.CURDIR}/${MACHINE}
AINC=	-I${.CURDIR}/${MACHINE}

LIB=	kern
NOPROFILE=

.if	(${MACHINE} == "i386")
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c 
SRCS+=	bcmp.s bzero.s ffs.s strcat.s strcmp.s strcpy.s strlen.s strncpy.c
SRCS+=	scanc.c skpc.c locc.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
.else
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c 
SRCS+=	bcmp.c bzero.c ffs.c strcat.c strcmp.c strcpy.c strlen.c strncpy.c
SRCS+=	scanc.c skpc.c locc.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
.endif


.include <bsd.lib.mk>
