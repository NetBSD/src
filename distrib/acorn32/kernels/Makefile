#	$NetBSD: Makefile,v 1.7 2002/04/07 04:21:22 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

KERNELS=	INSTALL 

RAMDISKDIR!=	cd $(.CURDIR)/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

CLEANFILES+=	netbsd.tmp netbsd.tmp.gz ${KERNELS:C/^.*$/netbsd.&.gz/g}

realall:	${KERNELS:C/^.*$/netbsd.&.gz/g}

.for kern in ${KERNELS}
netbsd.${kern}.gz: ${KERNOBJDIR}/${kern}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kern}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

.include <bsd.prog.mk>
