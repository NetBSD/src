#	$NetBSD: Makefile,v 1.1.2.2 1999/12/16 22:57:12 he Exp $

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
