#	$NetBSD: Makefile,v 1.9 2014/08/05 15:40:58 apb Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

FLOPPYBASE=	kern
FLOPPYSIZE=	2400
BOOTNAME!=	printf "USTAR.volsize.%o" ${FLOPPYSIZE}
FLOPPYFILES=	${BOOTNAME} netbsd
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
FLOPPY_NETBSD=	netbsd.gz
# XXXDISTRIB: use MI installboot
FLOPPYINSTBOOT=\
	"dd if=${MDEC}/fdboot_ustar of=@IMAGE@ bs=8k count=1 conv=sync,notrunc"

CLEANFILES+=	${BOOTNAME} netbsd.gz

${BOOTNAME}: ${MDEC}/boot
	@echo "Copying ${.ALLSRC} to ${.TARGET}"
	@rm -f ${.TARGET}
	@cp ${.ALLSRC} ${.TARGET}

netbsd.gz:	${KERNOBJDIR}/GENERIC/netbsd
	@rm -f ${.TARGET}
	${TOOL_GZIP_N} -9c ${.ALLSRC} > ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
