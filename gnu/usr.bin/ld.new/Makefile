#	$NetBSD: Makefile,v 1.7 1999/02/09 17:39:31 tv Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "i386" && ${OBJECT_FMT} == "ELF") || \
    (${MACHINE_ARCH} == "mipseb") || \
    (${MACHINE_ARCH} == "mipsel") || \
    (${MACHINE_ARCH} == "powerpc") || \
    (${MACHINE_ARCH} == "sparc" && ${OBJECT_FMT} == "ELF") || \
    (${MACHINE_ARCH} == "sparc64")
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
DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd
YHEADER=1

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/bfd; ${MAKE} print-objdir
SCRIPTDIR=	/usr/share/ldscripts

.PATH:		${DIST}/ld

##### alpha ######
EMULS.alpha=		elf64alpha
DEFAULT_EMUL.alpha=	elf64alpha

##### i386 #####
EMULS.i386=		i386nbsd elf_i386
.if ${OBJECT_FMT} == "ELF"
DEFAULT_EMUL.i386=	elf_i386
.else
DEFAULT_EMUL.i386=	i386nbsd
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

##### sparc #####
EMULS.sparc=		sparcnbsd elf32_sparc sun4
.if ${OBJECT_FMT} == "ELF"
DEFAULT_EMUL.sparc=	elf32_sparc
.else
DEFAULT_EMUL.sparc=	sparcnbsd
.endif

##### sparc64 #####
EMULS.sparc64=		elf64_sparc sparcnbsd elf32_sparc sun4
DEFAULT_EMUL.sparc64=	elf64_sparc

ALL_EMULS!=		(for i in ${EMULS.alpha} ${EMULS.i386} \
			${EMULS.mipseb} ${EMULS.mipsel} ${EMULS.powerpc} \
			${EMULS.sparc} ${EMULS.sparc64}; do echo $$i; done) | \
			sort | uniq

LIB_PATH=		${LIBDIR} # passed to genscripts

# The ldscripts need to be generated even if we don't build ld.
.for _EMUL_ in ${ALL_EMULS}
CLEANFILES+=		e${_EMUL_}.c
all: e${_EMUL_}.c
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

afterinstall:
	pax -rw ldscripts ${DESTDIR}${SCRIPTDIR}

cleanprog: __cleanldscripts
__cleanldscripts:
	rm -rf ldscripts

.include <bsd.prog.mk>

${OBJS}: ldemul-list.h
