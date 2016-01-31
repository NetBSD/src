# $NetBSD: Makefile,v 1.1 2016/01/31 18:56:49 christos Exp $
# Build a small disklabel (for tiny boot media)

SRCDIR=	${.CURDIR}/../../../sbin/disklabel

PROG=	disklabel
SRCS=	main.c dkcksum.c printlabel.c
#SRCS+=	interact.c
NOMAN=	# defined

CPPFLAGS+=	-DNO_INTERACT
CPPFLAGS+=	-DNATIVELABEL_ONLY

DPADD+= ${LIBUTIL}
LDADD+= -lutil

# these have additional requirements on the alignment of a partition
.if (${MACHINE} == "sparc") || (${MACHINE} == "sparc64") \
	|| (${MACHINE} == "sun3")
CPPFLAGS+= -DSTRICT_CYLINDER_ALIGNMENT
.endif

.if (${MACHINE} == "acorn32" || ${MACHINE} == "acorn26")
# Support FileCore boot block
CPPFLAGS+= -DUSE_ACORN
.endif

.if (${MACHINE_ARCH} == "alpha")
# alpha requires boot block checksum
CPPFLAGS+= -DALPHA_BOOTBLOCK_CKSUM
.endif

.if (${MACHINE_ARCH} == "vax")
# vax requires labels in alternative sectors on SMD disk
CPPFLAGS+= -DVAX_ALTLABELS
.endif

.include <bsd.prog.mk>

.PATH:	${SRCDIR}
