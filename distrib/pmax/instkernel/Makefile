#	$NetBSD: Makefile,v 1.1 1998/12/05 07:33:13 jonathan Exp $


KERN=	${.CURDIR}/../../../sys/arch/pmax/compile/RAMDISK/netbsd
RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-
CLEANFILES= netbsd.tmp netbsd.ecoff netbsd.ecoff.gz \
	    netbsd.aout netbsd.aout.gz

all:	netbsd.ram.gz

netbsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
	elf2ecoff netbsd.tmp netbsd.ecoff
	rm -f netbsd.ecoff.gz
	gzip netbsd.ecoff
	elf2aout netbsd.tmp netbsd.aout
	rm -f netbsd.aout.gz
	gzip netbsd.aout

clean cleandir distclean:
	rm -f ${CLEANFILES}

.include <bsd.own.mk>
.include <bsd.obj.mk>
