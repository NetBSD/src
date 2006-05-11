#	$NetBSD: Makefile,v 1.10 2006/05/11 23:16:31 mrg Exp $

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

.if ${HAVE_GCC} == 4
COPTS.mkarp.c+=	-Wno-pointer-sign
.endif
