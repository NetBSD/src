#	$NetBSD: Makefile,v 1.2 2002/08/18 12:39:42 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rescue
CRUNCHENV=	# empty - don't want SMALLPROG defined
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEDEPENDS=	${CRUNCHBIN}

.for f in bim edlabel fdisk ldconfig pdisk
PROG_${f}!=	cd ${NETBSDSRCDIR}/sbin/${f} && ${MAKE} -V PROG
.if (${PROG_${f}} != "")
LISTS+=		${.CURDIR}/list.${f}
.endif
.endfor

.if (${OBJECT_FMT} == "ELF")
PARSELISTENV+=	LDD_CMD=ldd_elf
.else
PARSELISTENV+=	LDD_CMD=ldd_aout
.endif


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.image"

realall: ${WORKBUILT} ${WORKSPEC}

install:
#	XXX: implement

.include <bsd.prog.mk>
