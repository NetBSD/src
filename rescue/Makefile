#	$NetBSD: Makefile,v 1.3 2002/10/03 01:11:37 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rescue
CRUNCHENV=	RESCUEDIR=/rescue
LISTS=		${.CURDIR}/list
TARGETDIR=	${DESTDIR}/rescue
PARSELISTENV+=  TARGETDIR=${TARGETDIR:Q}


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

.if ${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel"		# {

realall install:
	@echo "WARNING: src/rescue not built on ${MACHINE_ARCH} due to broken crunchgen"

.else									# } {

realall: ${CRUNCHBIN}

install: ${CRUNCHBIN}
	${PARSELIST} -v mode=install ${LISTS} | ${MAKE} -f - install

.endif									# }

.include <bsd.prog.mk>
