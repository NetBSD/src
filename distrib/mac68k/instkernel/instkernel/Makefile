#	$NetBSD: Makefile,v 1.1.2.3 2000/01/08 20:16:44 he Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

INSTALL_KERNEL?=INSTALL
KERNEL=${BSDSRCDIR}/sys/arch/mac68k/compile/${INSTALL_KERNEL}/netbsd

all:
	cp ${KERNEL} netbsd-${INSTALL_KERNEL}
	mdsetimage -T 0x0 -v netbsd-${INSTALL_KERNEL} ${RAMDISK}
# XXX Doesn't boot with this. Why?
#	strip netbsd-${INSTALL_KERNEL}
# XXX Reported not to boot on some machines.
#	gzip -9 -f netbsd-${INSTALL_KERNEL}
	gzip -f netbsd-${INSTALL_KERNEL}

clean cleandir distclean:
	rm -f netbsd*

release: all
	${INSTALL} -d -o root -g wheel -m 755 \
		${RELEASEDIR}/installation/instkernel/	
	${INSTALL} -c -o root -g wheel -m 644 netbsd-${INSTALL_KERNEL}.gz \
		${RELEASEDIR}/installation/instkernel/	

.include <bsd.own.mk>
.include <bsd.obj.mk>
