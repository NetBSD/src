#	$NetBSD: Makefile,v 1.8 1999/06/06 02:58:22 thorpej Exp $

PROG=	rarpd
SRCS=	rarpd.c mkarp.c
MAN=	rarpd.8

# Uncomment the following to require a boot file in TFTP_DIR
# to exist for a rarp reply to be sent.
# CPPFLAGS+=-DREQUIRE_TFTPBOOT -DTFTP_DIR=\"/tftpboot\"

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
