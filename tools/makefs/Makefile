#	$NetBSD: Makefile,v 1.6 2002/12/08 20:20:02 thorpej Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}makefs
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
