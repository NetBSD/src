#
#	$Id: Makefile,v 1.2 1993/09/02 01:00:42 jtc Exp $
#

.PATH: ${.CURDIR}/${MACHINE}
AINC+=	-I${.CURDIR}/${MACHINE}

LIBC=	${.CURDIR}/../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/${MACHINE}/string 
.PATH: ${LIBC}/net ${LIBC}/${MACHINE}/net 
.PATH: ${LIBC}/gen ${LIBC}/${MACHINE}/gen
AINC+=	-I${LIBC}/${MACHINE}

LIB=	kern
NOPROFILE=

.if	(${MACHINE} == "i386")
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c 
SRCS+=	bcmp.s bzero.s ffs.s strcat.s strcmp.s strcpy.s strlen.s strncpy.c
SRCS+=	scanc.c skpc.c locc.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	setjmp.s
.else
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c 
SRCS+=	bcmp.c bzero.c ffs.c strcat.c strcmp.c strcpy.c strlen.c strncpy.c
SRCS+=	scanc.c skpc.c locc.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
.endif


.include <bsd.lib.mk>
