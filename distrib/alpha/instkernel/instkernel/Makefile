#	$NetBSD: Makefile,v 1.4 1998/10/29 00:37:07 ross Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	strip netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd

.include <bsd.own.mk>
.include <bsd.obj.mk>
