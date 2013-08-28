#	$NetBSD: Makefile,v 1.14.116.1 2013/08/28 23:59:19 rmind Exp $
#
# NOTE: This program is *not* standalone, but a userlevel program -- this
#       program was run from within boot-only, NetBSD-as-bootloader as a
#       userlevel program, then in-kernel boot code loads another NetBSD
#	image and jumps to it.
#

PROG=	bootelf
SRCS=	alloc.c boot.c byteorder.c loadfile.c loadfile_elf32.c
NOMAN=	# defined

LIBSA=	${.CURDIR}/../../../../lib/libsa

.PATH: ${LIBSA}

CFLAGS+=	-ffreestanding
CPPFLAGS+=	-I${.CURDIR}/../../../.. -I${.OBJDIR}
LDSTATIC=	-static

${PROG}: ${OBJS}

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
