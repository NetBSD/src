#	$NetBSD: Makefile,v 1.45 2004/04/20 06:36:34 lukem Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SETSCMD=	cd ${.CURDIR} && \
		DESTDIR=${DESTDIR:Q} \
		MACHINE=${MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
		CKSUM=${TOOL_CKSUM:Q} MAKE=${MAKE:Q} MTREE=${TOOL_MTREE:Q} \
		MKTEMP=${TOOL_MKTEMP:Q} PAX=${TOOL_PAX:Q} HOST_SH=${HOST_SH:Q} \
		${HOST_SH}

.if ${MKX11} != "no"
MAKEFLIST_FLAGS=	-b
MAKESRCTARS_FLAGS=	-x ${X11SRCDIR}
.else
MAKEFLIST_FLAGS=
MAKESRCTARS_FLAGS=
.endif

.if !defined(MAKETARSETS)
MAKETARSETS!= ${SETSCMD} ./makeflist -l ${MAKEFLIST_FLAGS}
.endif

print_have_gcc3: .PHONY
	@echo ${HAVE_GCC3}

print_machine: .PHONY
	@echo ${MACHINE}

print_machine_arch: .PHONY
	@echo ${MACHINE_ARCH}

print_machine_cpu: .PHONY
	@echo ${MACHINE_CPU}

print_object_fmt: .PHONY
	@echo ${OBJECT_FMT}

print_toolchain_missing: .PHONY
	@echo "${TOOLCHAIN_MISSING}"

print_use_tools_toolchain: .PHONY
	@echo "${USE_TOOLS_TOOLCHAIN}"

print_x11_version: .PHONY
	${HOST_SH} -c '. ${.CURDIR}/sets.subr; echo $$x11_version'

# This target has debugging value only, really.
makeflist: .PHONY .PRECIOUS check_DESTDIR
	${SETSCMD} ./makeflist ${MAKEFLIST_FLAGS}

#
# SET BUILDING TARGETS
#

TARDIR=		${RELEASEDIR}/${MACHINE}/binary/sets
SOURCETARDIR=	${RELEASEDIR}/source/sets

.if ${MKUNPRIVED} == "no"
METALOG.unpriv=
.else
METALOG.unpriv=	-M ${METALOG}
.endif


checkflist: .PRECIOUS .PHONY check_DESTDIR
	${SETSCMD} ./checkflist \
	    ${MAKEFLIST_FLAGS} ${CHECKFLIST_FLAGS} ${METALOG.unpriv}

checkflist-x11: .PHONY check_DESTDIR
	${SETSCMD} ./checkflist -x ${CHECKFLIST_FLAGS}


maketars: .PRECIOUS .PHONY check_DESTDIR check_RELEASEDIR .WAIT \
		maketarsetup .WAIT ${MAKETARSETS:@.TARS.@do-${.TARS.}@}
	@true

maketarsetup: .EXEC
.if defined(DESTDIR) && ${DESTDIR} != ""
	${_MKMSG} "execute  checkflist"
	${MAKE} checkflist
.endif
	mkdir -p ${TARDIR}
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		rm -f ${TARDIR}/$$i ${TARDIR}/$$i.tmp; \
	done

makesetfiles: .PHONY
	${_MKMSG_CREATE} "set lists"
	${SETSCMD} ./maketars -S -d ${DESTDIR:S,^$,/,} ${METALOG.unpriv} \
	    -N ${NETBSDSRCDIR}/etc ${MAKEFLIST_FLAGS} -t ${TARDIR}

.for tar in ${MAKETARSETS}
do-${tar}: .PHONY
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} ${METALOG.unpriv} \
	    -N ${NETBSDSRCDIR}/etc -t ${TARDIR} ${tar} \
	|| rm -f ${TARDIR}/${tar}.tgz
.endfor


makesrctars: .PRECIOUS .PHONY check_RELEASEDIR
	mkdir -p ${SOURCETARDIR}
	${SETSCMD} ./makesrctars ${MAKESRCTARS_FLAGS} \
	    ${NETBSDSRCDIR} ${SOURCETARDIR}


makesums: .PRECIOUS .PHONY check_RELEASEDIR .WAIT \
		${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		mv ${TARDIR}/$$i.tmp ${TARDIR}/$$i; \
	done

.for tar in ${MAKETARSETS}
do-sum-${tar}: .PHONY do-${tar}
	${MAKESUMS} -t ${TARDIR} ${tar}.tgz
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		${TOOL_CAT} ${TARDIR}/$$i >> ${TARDIR}/$$i.tmp; \
	done
.endfor
.ORDER: ${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}


installsets: .PHONY check_DESTDIR
.if !defined(INSTALLDIR)
	@echo "setenv INSTALLDIR before doing that!"
	@false
.endif
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.unpriv} -N ${NETBSDSRCDIR}/etc ${MAKEFLIST_FLAGS} \
	    -i ${INSTALLDIR} ${INSTALLSETS}


#
# MAIN ENTRY POINTS
#

sets: .PHONY maketars .WAIT makesums
	@true

sourcesets: .PHONY makesrctars
	@true

.include <bsd.sys.mk>
