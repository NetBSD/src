#	$NetBSD: Makefile,v 1.6 1999/06/25 22:32:40 nathanw Exp $

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
	rm -f netbsd netbsd.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
