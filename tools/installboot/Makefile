#	$NetBSD: Makefile,v 1.7 2002/12/08 20:20:01 thorpej Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}installboot
HOST_SRCDIR=	usr.sbin/installboot

DPSRCS+=	ufs.stamp
HOST_CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../mips-elf2ecoff
CLEANFILES+=	ufs

.include "${.CURDIR}/../Makefile.host"

$(OBJS): ufs.stamp

ufs.stamp:
	-rm -f ufs
	ln -f -s ${.CURDIR}/../../sys/ufs ufs
	@touch $@
