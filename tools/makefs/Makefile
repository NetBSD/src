#	$NetBSD: Makefile,v 1.2 2002/01/10 05:03:45 lukem Exp $

HOSTPROGNAME=	nbmakefs
HOST_SRCDIR=	usr.sbin/makefs

.include "${.CURDIR}/../Makefile.host"

HOST_CPPFLAGS+=	-I.
CLEANFILES+=	ufs

${SRCS}:	ufs

ufs:
	ln -fs ${.CURDIR}/../../sys/ufs

