#	$NetBSD: Makefile,v 1.1 2001/01/16 16:54:00 nonaka Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.subdir.mk>

KERN=		${KERNOBJDIR}/GENERIC/netbsd
KERNEL=		netbsd.GENERIC.gz

CLEANFILES+=	${KERNEL} netbsd.tmp netbsd.tmp.gz

all:	${KERNEL}

${KERNEL}: ${KERN}
	cp ${KERN} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

.ifndef RELEASEDIR
release:
	@echo set RELEASEDIR first!
	@false
.else
release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNEL} ${RELEASEDIR}/binary/kernel
.endif

clean cleandir distclean:
	rm -f *.core ${CLEANFILES}

.include <bsd.obj.mk>
