#	$NetBSD: Makefile,v 1.9 2000/04/13 09:12:16 itojun Exp $

PROG=	rarpd
SRCS=	rarpd.c mkarp.c
MAN=	rarpd.8

# Uncomment the following to require a boot file in TFTP_DIR
# to exist for a rarp reply to be sent.
# CPPFLAGS+=-DREQUIRE_TFTPBOOT -DTFTP_DIR=\"/tftpboot\"
CPPFLAGS+=-DHAVE_IFADDRS_H

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
