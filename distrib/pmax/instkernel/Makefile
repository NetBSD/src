#	$NetBSD: Makefile,v 1.1.2.1 1999/04/05 15:07:18 simonb Exp $


KERN=	${.CURDIR}/../../../sys/arch/pmax/compile/RAMDISK/netbsd
RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-
CLEANFILES= netbsd.tmp netbsd.ecoff netbsd.ecoff.gz \
	    netbsd.aout netbsd.aout.gz

all:	netbsd.gz

netbsd.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	elf2ecoff netbsd netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip -9 netbsd.ecoff
	rm -f netbsd.gz
	gzip -9 netbsd

clean cleandir distclean:
	rm -f ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
