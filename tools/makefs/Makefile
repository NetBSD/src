#	$NetBSD: Makefile,v 1.3 2002/01/31 22:43:51 tv Exp $

HOSTPROGNAME=	nbmakefs
HOST_SRCDIR=	usr.sbin/makefs

DPSRCS+=	ufs.stamp
HOST_CPPFLAGS+=	-I.
CLEANFILES+=	ufs

.include "${.CURDIR}/../Makefile.host"

ufs.stamp:
	-rm -f ufs
	ln -fs ${.CURDIR}/../../sys/ufs ufs
	@touch $@
