#	$NetBSD: Makefile,v 1.2 2002/03/05 21:16:51 thorpej Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

KERNELS=	INSTALL 

STRIP?=		strip

RAMDISKDIR!=	cd $(.CURDIR)/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk-${REV}.fs

CLEANFILES+=	netbsd.tmp ${KERNELS:C/^.*$/netbsd.&.gz/g}
MDSETIMAGE?=	mdsetimage

all:	${KERNELS:C/^.*$/netbsd.&.gz/g}

.for kern in ${KERNELS}
netbsd.${kern}.gz: ${KERNOBJDIR}/${kern}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kern}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

.include <bsd.obj.mk>
