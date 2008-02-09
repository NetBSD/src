#	$NetBSD: Makefile,v 1.21 2008/02/09 02:37:21 mrg Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
# XXX
.if ${MACHINE_ARCH} != "m68000"
DBG=		-Os
.endif

CRUNCHGEN_FLAGS=-d "${DBG}"

RESCUEDIR=	/rescue
CRUNCHBIN=	rescue
CRUNCHENV=	RESCUEDIR=${RESCUEDIR}
SMALLPROG=	0
LISTS=		${.CURDIR}/list
TARGETDIR=	${DESTDIR}/rescue
PARSELISTENV+=  TARGETDIR=${TARGETDIR:Q}

.for f in edlabel ldconfig pdisk
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
CRUNCHENV+=	MKKERBEROS=no		# for ssh
.endif

.if (${OBJECT_FMT} == "ELF")
PARSELISTENV+=	LDD_CMD=ldd_elf
.else
PARSELISTENV+=	LDD_CMD=ldd_aout
.endif

#	Specially built objects to override the behaviour of
#	various library functions
#

OVERRIDEOBJS=	rcmd.o
.PATH:	${NETBSDSRCDIR}/lib/libc/net
CPPFLAGS.rcmd.c+=-I${NETBSDSRCDIR}/lib/libc/include -DRESCUEDIR=\"${RESCUEDIR}\"

LIBOVERRIDE=	liboverride.o
${LIBOVERRIDE}: ${OVERRIDEOBJS}
	${_MKTARGET_LINK}
	${LD} -r -o $@ ${OVERRIDEOBJS}

CLEANFILES+=	${OVERRIDEOBJS} ${LIBOVERRIDE}

${CRUNCHBIN}:	${LIBOVERRIDE}


#	The primary target ...
#

CLEANFILES+=	rescue.unstripped

.include "${DISTRIBDIR}/common/Makefile.crunch"

realall: ${CRUNCHBIN}

install: ${CRUNCHBIN}
# XXX this MKMSG doesn't line up
	${_MKMSG} "populate ${TARGETDIR}"
	${PARSELIST} -v mode=install ${LISTS} | ${MAKE} -f - install

.include <bsd.prog.mk>
