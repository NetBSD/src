#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:33 mrg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=           cpp0
SRCS=		${G_CCCP_OBJS:.o=.c}

CPPFLAGS+=	-I${GCCARCH} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DGCC_INCLUDE_DIR=\"\" -DPREFIX=\"/usr\"

BINDIR=		/usr/libexec

.include <bsd.prog.mk>

# Don't auto-frob .y or .l files.
.l.c .y.c .y.h:
	@true

.PATH: ${DIST}/gcc
