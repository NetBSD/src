#	$NetBSD: Makefile,v 1.4 2002/04/19 07:19:09 lukem Exp $

HOSTPROGNAME=	nbinstallboot
HOST_SRCDIR=	usr.sbin/installboot
HOST_CPPFLAGS=	-I${.CURDIR} -I${.CURDIR}/../mips-elf2ecoff

.include "${.CURDIR}/../Makefile.host"

$(OBJS): ufs.stamp

ufs.stamp:
	-rm -f ufs
	ln -fs ${.CURDIR}/../../sys/ufs ufs
	@touch $@
