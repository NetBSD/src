#	$NetBSD: Makefile,v 1.32 1999/04/09 16:00:17 kleink Exp $
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

# these have additional requirements on the alignment of a partition
.if (${MACHINE} == "sparc") || (${MACHINE} == "sparc64") \
	|| (${MACHINE} == "sun3")
CPPFLAGS+= -DSTRICT_CYLINDER_ALIGNMENT
.endif

.include <bsd.prog.mk>
