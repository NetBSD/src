#	$NetBSD: Makefile,v 1.65 2009/12/05 16:29:11 pooka Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	main.c dkcksum.c interact.c printlabel.c
MAN=	disklabel.5 disklabel.8
.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

# these have additional requirements on the alignment of a partition
.if (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "sparc64") \
	|| (${MACHINE} == "sun3")
CPPFLAGS+= -DSTRICT_CYLINDER_ALIGNMENT
.endif

.if ( 0 \
	|| ${MACHINE} == "acorn26" \
	|| ${MACHINE} == "acorn32" \
	|| ${MACHINE} == "amd64" \
	|| ${MACHINE} == "arc" \
	|| ${MACHINE} == "cats" \
	|| ${MACHINE} == "cobalt" \
	|| ${MACHINE} == "dreamcast" \
	|| ${MACHINE} == "evbarm" \
	|| ${MACHINE} == "evbsh3" \
	|| ${MACHINE} == "hpcarm" \
	|| ${MACHINE} == "hpcmips" \
	|| ${MACHINE} == "hpcsh" \
	|| ${MACHINE} == "i386" \
	|| ${MACHINE} == "iyonix" \
	|| ${MACHINE} == "landisk" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "mmeye" \
	|| ${MACHINE} == "netwinder" \
	|| ${MACHINE} == "prep" \
	|| ${MACHINE} == "shark" \
	|| ${MACHINE} == "zaurus" \
    )
# use MBR partition info
CPPFLAGS+= -DUSE_MBR
# recognize old MBR partition ID for a while
CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
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
