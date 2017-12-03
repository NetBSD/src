#	$NetBSD: Makefile,v 1.6.12.1 2017/12/03 11:35:58 jdolecek Exp $

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
