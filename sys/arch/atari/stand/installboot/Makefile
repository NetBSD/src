#	$NetBSD: Makefile,v 1.10 2022/05/11 10:36:52 rin Exp $

WARNS=	4
PROG=	installboot
NOMAN=	# defined
SRCS=	installboot.c disklabel.c
BINDIR=	/usr/mdec
.ifndef SMALLPROG
CPPFLAGS+=	-DCHECK_OS_BOOTVERSION -DNO_USAGE -DSUPPORT_FD
LDADD=	-lkvm
.endif

# XXX needs proper struct or union to calculate boot sector uint16_t cksums
CFLAGS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
