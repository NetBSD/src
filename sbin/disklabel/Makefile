#	$NetBSD: Makefile,v 1.73 2018/01/24 09:04:42 skrll Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	main.c dkcksum.c interact.c printlabel.c bswap.c
MAN=	disklabel.5 disklabel.8
.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

# these have additional requirements on the alignment of a partition
.if (${MACHINE} == "sparc") || (${MACHINE} == "sparc64") \
	|| (${MACHINE} == "sun3")
CPPFLAGS+= -DSTRICT_CYLINDER_ALIGNMENT
.endif

.if ${MACHINE} == "acorn32"
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
