#	$NetBSD: Makefile,v 1.11.12.1 2008/09/18 04:30:14 wrstuden Exp $

USE_FORT?= yes	# network server

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

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.mkarp.c+=	-Wno-pointer-sign
.endif
