#	$NetBSD: Makefile,v 1.29.2.1 1998/11/23 08:11:42 cgd Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c interact.c
MAN=	disklabel.5 disklabel.8
LDADD+= -lutil
DPADD+= ${LIBUTIL}

# recognize old partition ID for a while
.if (${MACHINE} == "i386")
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
.endif

.if (${MACHINE} == "hp300") || (${MACHINE} == "vax") \
	|| (${MACHINE} == "arm32")
CPPFLAGS+= -DNUMBOOT=1
.endif

.include <bsd.prog.mk>
