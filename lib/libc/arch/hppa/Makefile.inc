#	$NetBSD: Makefile.inc,v 1.2 2002/07/10 04:29:06 thorpej Exp $

#	$OpenBSD: Makefile.inc,v 1.3 1999/12/25 06:02:10 mickey Exp $

KMINCLUDES=	arch/hppa/DEFS.h arch/hppa/SYS.h prefix.h
KMSRCS=	bcmp.S bzero.S bcopy.m4 ffs.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S

#SRCS+=	memmove.S
#CLEANFILES+=	memmove.S
SRCS+= bcopy.c
CLEANFILES+=	cerror.S

cerror.S:
	@echo > cerror.S

memmove.S:	${LIBCSRCDIR}/arch/hppa/string/bcopy.m4
	@echo 'building ${.TARGET} from ${.ALLSRC}'
	@m4 -DNAME=bcopy ${.ALLSRC} > ${.TARGET}

