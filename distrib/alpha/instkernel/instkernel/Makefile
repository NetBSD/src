#	$NetBSD: Makefile,v 1.3 1998/09/05 14:46:14 lukem Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}

clean cleandir distclean:
	rm -f netbsd

.include <bsd.own.mk>
.include <bsd.obj.mk>
