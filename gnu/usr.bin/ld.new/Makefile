#	$NetBSD: Makefile,v 1.22 2001/01/09 08:52:59 enami Exp $

# for OBJECT_FMT, UNPRIVILEGED
.include <bsd.own.mk>

.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
ELF_PLEASE=
.endif

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386" && defined(ELF_PLEASE)) || \
    (${MACHINE_ARCH} == "m68k" && defined(ELF_PLEASE)) || \
    (${MACHINE_ARCH} == "mipseb") || \
    (${MACHINE_ARCH} == "mipsel") || \
    (${MACHINE_ARCH} == "powerpc") || \
    (${MACHINE_ARCH} == "sh3") || \
    (${MACHINE_ARCH} == "sparc" && defined(ELF_PLEASE))
PROG=		ld
MAN=		ld.1
SRCS=		ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c ldmain.c ldmisc.c \
		ldver.c ldwrite.c lexsup.c mri.c ldcref.c ldgram.y ldlex.l
DPSRCS=		ldemul-list.h
.else
NOPROG=
.endif

CPPFLAGS+=	-I. -I${.CURDIR} -I${BFDOBJ} -I${DIST}/ld \
		-I${DIST}/bfd -I${DIST}/include \
		-DDEFAULT_EMULATION='"${DEFAULT_EMUL.${MACHINE_ARCH}}"' \
		-DSCRIPTDIR=\"${SCRIPTDIR}\" \
		-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"' \
		-DEMULATION_LIST='${EMULS:S/^/\&ld_/:S/$/_emulation,/} 0'
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a
YHEADER=1

TEXINFO=	ld.texi
INFOFLAGS=	-I${DIST}/ld -I${DIST}/bfd/doc

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
SCRIPTDIR=	/usr/share/ldscripts

.PATH:		${DIST}/ld

##### alpha ######
EMULS.alpha=		elf64alpha
DEFAULT_EMUL.alpha=	elf64alpha

##### i386 #####
EMULS.i386=		i386nbsd elf_i386
.if defined(ELF_PLEASE)
DEFAULT_EMUL.i386=	elf_i386
.else
DEFAULT_EMUL.i386=	i386nbsd
.endif

##### m68k #####
EMULS.m68k=		m68knbsd m68kelfnbsd
.if defined(ELF_PLEASE)
DEFAULT_EMUL.m68k=	m68kelfnbsd
.else
DEFAULT_EMUL.m68k=	m68knbsd
.endif

##### mipseb #####
EMULS.mipseb=		elf32lmip elf32bmip
DEFAULT_EMUL.mipseb=	elf32bmip

##### mipsel #####
EMULS.mipsel=		elf32lmip elf32bmip
DEFAULT_EMUL.mipsel=	elf32lmip

##### powerpc #####
EMULS.powerpc=		elf32ppc
DEFAULT_EMUL.powerpc=	elf32ppc

##### sh3 #####
EMULS.sh3=		shunx shlunx elf32shunx elf32shlunx
.if defined(ELF_PLEASE)
DEFAULT_EMUL.sh3=	elf32shunx
.else
DEFAULT_EMUL.sh3=	shunx
.endif

##### sparc #####
EMULS.sparc=		sparcnbsd elf32_sparc sun4
.if defined(ELF_PLEASE)
DEFAULT_EMUL.sparc=	elf32_sparc
.else
DEFAULT_EMUL.sparc=	sparcnbsd
.endif

##### sparc64 #####
EMULS.sparc64=		elf64_sparc sparcnbsd elf32_sparc sun4
DEFAULT_EMUL.sparc64=	elf64_sparc

ALL_EMULS!=		(for i in ${EMULS.alpha} ${EMULS.i386} ${EMULS.m68k} \
			${EMULS.mipseb} ${EMULS.mipsel} ${EMULS.powerpc} \
			${EMULS.sh3} ${EMULS.sparc} ${EMULS.sparc64}; \
			do echo $$i; done) | \
			sort | uniq

LIB_PATH=		${LIBDIR} # passed to genscripts

# The ldscripts need to be generated even if we don't build ld.
.for _EMUL_ in ${ALL_EMULS}
CLEANFILES+=		e${_EMUL_}.c
realall: e${_EMUL_}.c
e${_EMUL_}.c: ${DIST}/ld/genscripts.sh ${DIST}/ld/emulparams/${_EMUL_}.sh
	sh ${DIST}/ld/genscripts.sh ${DIST}/ld ${LIBDIR} \
		${MACHINE_GNU_ARCH}-netbsd ${MACHINE_GNU_ARCH}-netbsd \
		${MACHINE_GNU_ARCH}-netbsd ${_EMUL_} \
		"" ${_EMUL_}
.endfor

.if !defined(NOPROG)
#.if defined(UNIFIED_LD)
#EMULS=			${ALL_EMULS} # XXX not quite functional yet
#.else
EMULS=			${EMULS.${MACHINE_ARCH}}
#.endif
.for _EMUL_ in ${EMULS}
SRCS+=			e${_EMUL_}.c
.endfor

ldemul-list.h: Makefile
	@rm -f $@
	@echo updating $@
	@for emul in ${EMULS}; do \
		echo "extern ld_emulation_xfer_type ld_$${emul}_emulation;" >>$@; \
	done
.endif

.include <bsd.own.mk>

afterinstall:
.if ${MKSHARE} != "no"
	(cd ldscripts ; pax -rw . ${DESTDIR}${SCRIPTDIR})
.if !defined(UNPRIVILEGED)
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${SCRIPTDIR}
.endif
	find ${DESTDIR}${SCRIPTDIR} -type f | xargs chmod a=rX
	find ${DESTDIR}${SCRIPTDIR} -type d | xargs chmod u=rwX,go=rX
.endif

cleanprog: __cleanldscripts
__cleanldscripts:
	rm -rf ldscripts

.include <bsd.info.mk>
.include <bsd.prog.mk>

${OBJS}: ldemul-list.h
