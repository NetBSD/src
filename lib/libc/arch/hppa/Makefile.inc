#	$NetBSD: Makefile.inc,v 1.4 2004/07/01 06:50:04 skrll Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

KMINCLUDES=	arch/hppa/DEFS.h arch/hppa/SYS.h prefix.h
KMSRCS=	bcmp.S bzero.S bcopy.m4 ffs.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S

#SRCS+=	memmove.S
#CLEANFILES+=	memmove.S
SRCS+= bcopy.c

#memmove.S:	${.CURDIR}/arch/hppa/string/bcopy.m4
#	@echo 'building ${.TARGET} from ${.ALLSRC}'
#	@m4 -DNAME=bcopy ${.ALLSRC} > ${.TARGET}

