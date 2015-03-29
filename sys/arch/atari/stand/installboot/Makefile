#	$NetBSD: Makefile,v 1.6.28.2 2015/03/29 07:42:53 snj Exp $

WARNS=	4
PROG=	installboot
NOMAN=	# defined
SRCS=	installboot.c disklabel.c
BINDIR=	/usr/mdec
.ifndef SMALLPROG
CPPFLAGS+=	-DCHECK_OS_BOOTVERSION
LDADD=	-lkvm
.endif

# XXX needs proper struct or union to calculate boot sector uint16_t cksums
CFLAGS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
