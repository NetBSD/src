#	$NetBSD: Makefile,v 1.11.4.1 2006/04/22 11:37:44 simonb Exp $
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

CLEANFILES+=	machine

realdepend realall: machine

machine::
	-rm -f $@
	ln -s ${.CURDIR}/../../../../arch/${MACHINE}/include $@

${PROG}: ${OBJS} machine

.include <bsd.prog.mk>
