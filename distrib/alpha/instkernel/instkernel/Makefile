#	$NetBSD: Makefile,v 1.1 1998/02/01 01:58:51 thorpej Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}

.include <bsd.own.mk>
.include <bsd.obj.mk>
