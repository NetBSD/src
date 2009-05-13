#	$NetBSD: Makefile,v 1.19.42.1 2009/05/13 19:17:38 jym Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

# Stuff an arbitrary kernel onto a ustarfs boot floppy set.
#
# Optional variables available:
#	FLOPPYKERNEL	Path to kernel to stuff onto the floppy.
#			[ ${KERNOBJDIR}/GENERIC/netbsd ]
#
#	FLOPPYBASE	Basename of floppies (written as "${FLOPPYBASE}N.fs")
#			[ "generic" ]
#
#	FLOPPYSIZE	Size of a floppy.  [ 2880 ]
#
#	FLOPPYMAX	Maximum number of floppies allowed in the set.  [1]
#
FLOPPYKERNEL?=	${KERNOBJDIR}/GENERIC/netbsd
FLOPPYBASE?=	generic
FLOPPYMAX?=	1

FLOPPY_RELEASEDIR?=	installation/misc

.if defined(FLOPPYSIZE) && (${FLOPPYSIZE} != 2880)
FLOPPYMETAFILE!=printf "USTAR.volsize.%o" ${FLOPPYSIZE}
.else
FLOPPYSIZE=	2880
.endif

FLOPPYFILES=	boot ${FLOPPYMETAFILE} netbsd

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=	"${TOOL_INSTALLBOOT} -v -m ${MACHINE} -t raw -b 17 @IMAGE@ ${FLOPPY_BOOT}"

netbsd: ${FLOPPYKERNEL}
	@echo "Copying stripped and gzipped ${.ALLSRC} to ${.TARGET}"
	@rm -f ${.TARGET} ${.TARGET}.tmp
	cp ${.ALLSRC} ${.TARGET}.tmp
	${STRIP} ${.TARGET}.tmp
	gzip -9nf ${.TARGET}.tmp
	mv ${.TARGET}.tmp.gz ${.TARGET}

CLEANFILES+=	netbsd netbsd.tmp netbsd.tmp.gz

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
