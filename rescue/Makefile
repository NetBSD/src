#	$NetBSD: Makefile,v 1.6 2002/10/17 01:49:18 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rescue
CRUNCHENV=	RESCUEDIR=/rescue
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

.if (${OBJECT_FMT} == "ELF")
PARSELISTENV+=	LDD_CMD=ldd_elf
.else
PARSELISTENV+=	LDD_CMD=ldd_aout
.endif


.include "${DISTRIBDIR}/common/Makefile.crunch"

# XXX Make sure to update distrib/sets/makeflist when this conditional
# XXX is changed or removed.
.if ${MACHINE_CPU} == "mips"						# {

realall install:
	@echo "WARNING: src/rescue not built on ${MACHINE_ARCH} due to broken crunchgen"

.else									# } {

realall: ${CRUNCHBIN}

install: ${CRUNCHBIN}
	${PARSELIST} -v mode=install ${LISTS} | ${MAKE} -f - install

.endif									# }

.include <bsd.prog.mk>
