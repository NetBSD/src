#	$NetBSD: Makefile,v 1.5 2002/04/22 15:18:57 lukem Exp $

HOSTPROGNAME=	nbinstallboot
HOST_SRCDIR=	usr.sbin/installboot

DPSRCS+=	ufs.stamp
HOST_CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../mips-elf2ecoff
CLEANFILES+=	ufs

.include "${.CURDIR}/../Makefile.host"

$(OBJS): ufs.stamp

ufs.stamp:
	-rm -f ufs
	ln -fs ${.CURDIR}/../../sys/ufs ufs
	@touch $@
