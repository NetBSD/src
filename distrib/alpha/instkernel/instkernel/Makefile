#	$NetBSD: Makefile,v 1.8 2000/01/19 16:40:11 tsutsui Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL?=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd
STRIPPROG?=	strip		# make STRIPPROG=echo can be useful

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	${STRIPPROG} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
