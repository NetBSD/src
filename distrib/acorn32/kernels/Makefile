#	$NetBSD: Makefile,v 1.8 2002/04/10 15:26:44 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP}/distrib/Makefile.inc"

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
