#	$NetBSD: Makefile,v 1.1 1999/03/26 08:34:37 ender Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL=${BSDSRCDIR}/sys/arch/mac68k/compile/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	mdsetimage -T 0x0 -v netbsd ${RAMDISK}
	strip netbsd
	gzip -f netbsd

clean cleandir distclean:
	rm -f netbsd

.include <bsd.own.mk>
.include <bsd.obj.mk>
