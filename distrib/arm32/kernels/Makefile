#	$NetBSD: Makefile,v 1.2 2001/10/21 23:23:19 jmc Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

KERNELS=	A7INST CATSINST RPCINST SHARKINST

STRIP?=		strip

RAMDISK!=	cd $(.CURDIR)/../ramdisk/; \
		    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-${REV}.fs\n" | \
		    ${MAKE} -s -f-

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
