#	$NetBSD: Makefile,v 1.4 2002/03/05 15:08:27 pk Exp $

HOSTPROGNAME=	nbmakefs
HOST_SRCDIR=	usr.sbin/makefs

DPSRCS+=	ufs.stamp
HOST_CPPFLAGS+=	-I.
CLEANFILES+=	ufs

.include "${.CURDIR}/../Makefile.host"

$(OBJS): ufs.stamp

ufs.stamp:
	-rm -f ufs
	ln -fs ${.CURDIR}/../../sys/ufs ufs
	@touch $@
