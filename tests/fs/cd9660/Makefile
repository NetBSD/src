# $NetBSD: Makefile,v 1.2 2024/04/28 14:39:22 rillig Exp $

TESTSDIR=		${TESTSBASE}/fs/cd9660
FILESDIR=		${TESTSDIR}

TESTS_SH=		t_high_ino_big_file
PROGS=			h_hexdump_r
MAN.h_hexdump_r=	# none
BINDIR.h_hexdump_r=	${TESTSDIR}
FILES=			pr_48787.image.hex

.include <bsd.test.mk>
