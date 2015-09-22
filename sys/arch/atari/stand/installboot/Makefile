#	$NetBSD: Makefile,v 1.8.2.1 2015/09/22 12:05:38 skrll Exp $

WARNS=	4
PROG=	installboot
NOMAN=	# defined
SRCS=	installboot.c disklabel.c
BINDIR=	/usr/mdec
.ifndef SMALLPROG
CPPFLAGS+=	-DCHECK_OS_BOOTVERSION -DNO_USAGE
LDADD=	-lkvm
.endif

# XXX needs proper struct or union to calculate boot sector uint16_t cksums
CFLAGS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
