#	$NetBSD: Makefile,v 1.31.2.1 1999/06/21 23:19:57 perry Exp $
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

# recognize old partition ID for a while
.if (${MACHINE} == "sparc") || (${MACHINE} == "sparc64") \
	|| (${MACHINE} == "sun3")
CPPFLAGS+= -DSTRICT_CYLINDER_ALIGNMENT
.endif

.if (${MACHINE} == "alpha")
# read in the old boot area even though we don't support writing the boot
# area with disklabel(8).
CPPFLAGS+= -DSAVEBOOTAREA
.endif

.include <bsd.prog.mk>
