#	$NetBSD: Makefile,v 1.5 2002/05/31 05:50:03 matt Exp $

HOSTPROGNAME=	nbmakefs
HOST_SRCDIR=	usr.sbin/makefs

DPSRCS+=	ufs.stamp
HOST_CPPFLAGS+=	-I.
CLEANFILES+=	ufs

.include "${.CURDIR}/../Makefile.host"

$(OBJS): ufs.stamp

ufs.stamp:
	-rm -f ufs
	ln -f -s ${.CURDIR}/../../sys/ufs ufs
	@touch $@
