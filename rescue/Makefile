#	$NetBSD: Makefile,v 1.14 2005/01/10 10:04:31 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

RESCUEDIR=	/rescue
CRUNCHBIN=	rescue
CRUNCHENV=	RESCUEDIR=${RESCUEDIR}
SMALLPROG=	0
LISTS=		${.CURDIR}/list
TARGETDIR=	${DESTDIR}/rescue
PARSELISTENV+=  TARGETDIR=${TARGETDIR:Q}


.for f in bim edlabel fdisk ldconfig pdisk
PROG_${f}!=	cd ${NETBSDSRCDIR}/sbin/${f} && ${MAKE} -V PROG
.if (${PROG_${f}} != "")
LISTS+=		${.CURDIR}/list.${f}
.endif
.endfor

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif

.if ${MKCRYPTO} != "no"
LISTS+=		${.CURDIR}/list.crypto
.endif

.if (${OBJECT_FMT} == "ELF")
PARSELISTENV+=	LDD_CMD=ldd_elf
.else
PARSELISTENV+=	LDD_CMD=ldd_aout
.endif

CLEANFILES+=	rescue.unstripped

.include "${DISTRIBDIR}/common/Makefile.crunch"

realall: ${CRUNCHBIN}

install: ${CRUNCHBIN}
	${PARSELIST} -v mode=install ${LISTS} | ${MAKE} -f - install

.include <bsd.prog.mk>
