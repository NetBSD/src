#	$NetBSD: Makefile,v 1.2 1998/05/03 16:42:37 ross Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}

clean cleandir:
	rm -f netbsd

.include <bsd.own.mk>
.include <bsd.obj.mk>
