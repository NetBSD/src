#	$NetBSD: Makefile,v 1.4 2002/03/06 23:50:57 lukem Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

KERNELS=	A7INST CATSINST RPCINST SHARKINST

STRIP?=		strip

RAMDISKDIR!=	cd $(.CURDIR)/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk-${DISTRIBREV}.fs

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
