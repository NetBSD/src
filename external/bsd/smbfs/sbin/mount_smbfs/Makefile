#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:52:13 tls Exp $

WARNS?=	3	# XXX: sign-compare issues

BINDIR=/sbin
.include "../../Makefile.inc"

PROG=	mount_smbfs
MAN=	mount_smbfs.8
SRCS=	mount_smbfs.c

CPPFLAGS+= -I${SMBDIST}/mount_smbfs

.PATH: ${SMBDIST}/mount_smbfs

DPADD+=${LIBUTIL}
LDADD+=-lutil

PROGDPLIBS += smb ${.CURDIR}/../../lib/libsmb

.if ${MKSHARE} != "no"
.PATH:	${SMBDIST}/examples
FILESDIR=	/usr/share/examples/smbfs
FILES=		dot.nsmbrc
.endif

.include <bsd.prog.mk>
