#	$NetBSD: Makefile,v 1.7 1999/09/02 23:39:43 ross Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL?=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd
STRIP?=	strip		# make STRIP=echo can be useful

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	${STRIP} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
