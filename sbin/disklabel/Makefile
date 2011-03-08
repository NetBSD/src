#	$NetBSD: Makefile,v 1.61.6.1 2011/03/08 17:05:32 riz Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/17/94

PROG=	disklabel
SRCS=	main.c dkcksum.c interact.c printlabel.c
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

.if ( 0 \
	|| ${MACHINE} == "acorn26" \
	|| ${MACHINE} == "acorn32" \
	|| ${MACHINE} == "amd64" \
	|| ${MACHINE} == "arc" \
	|| ${MACHINE} == "cats" \
	|| ${MACHINE} == "cobalt" \
	|| ${MACHINE} == "evbarm" \
	|| ${MACHINE} == "hpcarm" \
	|| ${MACHINE} == "hpcmips" \
	|| ${MACHINE} == "hpcsh" \
	|| ${MACHINE} == "i386" \
	|| ${MACHINE} == "iyonix" \
	|| ${MACHINE} == "landisk" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "netwinder" \
	|| ${MACHINE} == "playstation2" \
	|| ${MACHINE} == "prep" \
	|| ${MACHINE} == "shark" \
	|| ${MACHINE} == "zaurus" \
    )
# use MBR partition info
CPPFLAGS+= -DUSE_MBR
# do not recognize old MBR partition ID any more
#CPPFLAGS+= -DCOMPAT_386BSD_MBRPART
.endif

.if (${MACHINE} == "acorn32" || ${MACHINE} == "acorn26")
# Support FileCore boot block
CPPFLAGS+= -DUSE_ACORN
.endif

.include <bsd.prog.mk>
