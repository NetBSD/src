#	$NetBSD: Makefile,v 1.1 2001/07/18 14:01:14 tv Exp $

.include <bsd.own.mk>

PROG=           cpp0
SRCS=		${G_CCCP_OBJS:.o=.c}
MKMAN=		no

CPPFLAGS+=	-I${GCCARCH} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DGCC_INCLUDE_DIR=\"\" -DPREFIX=\"/usr\"

BINDIR=		/usr/libexec

.include <bsd.prog.mk>

.PATH: ${DIST}/gcc
