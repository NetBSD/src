#	$NetBSD: Makefile,v 1.11 2022/07/03 16:16:50 tsutsui Exp $

WARNS=	4
PROG=	installboot
NOMAN=	# defined
SRCS=	installboot.c disklabel.c
BINDIR=	/usr/mdec
.ifndef SMALLPROG
CPPFLAGS+=	-DCHECK_OS_BOOTVERSION -DUSAGE -DSUPPORT_FD
LDADD=	-lkvm
.endif

# XXX needs proper struct or union to calculate boot sector uint16_t cksums
CFLAGS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
