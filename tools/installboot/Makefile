#	$NetBSD: Makefile,v 1.5.2.1 2002/05/31 11:29:17 tv Exp $

HOSTPROGNAME=	nbinstallboot
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
