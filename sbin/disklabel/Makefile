#	$NetBSD: Makefile,v 1.34 1999/10/11 05:28:04 shin Exp $
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

.if (${MACHINE} == "alpha")
# read in the old boot area even though we don't support writing the boot
# area with disklabel(8).
CPPFLAGS+= -DSAVEBOOTAREA
.endif

.if (${MACHINE} == "hpcmips")
# recognize old partition ID for a while
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
# use MBR partition info
CPPFLAGS+= -DUSE_MBR
.endif

.include <bsd.prog.mk>
