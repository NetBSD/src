#	$NetBSD: Makefile,v 1.35 2003/12/22 04:56:53 jmc Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SETSENV=	DESTDIR=${DESTDIR:Q} \
		MACHINE=${MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
		CKSUM=${TOOL_CKSUM:Q} MAKE=${MAKE:Q} MTREE=${TOOL_MTREE:Q} \
		MKTEMP=${TOOL_MKTEMP:Q} PAX=${TOOL_PAX:Q} 

.if !defined(MAKETARSETS)
MAKETARSETS!=	${SETSENV} ${HOST_SH} -c '. ${.CURDIR}/sets.defaults; echo $${lists}'
.endif

print_have_gcc3:
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
.if defined(USE_XF86_4) && (${USE_XF86_4} != no)
	@echo 4
.else
	@echo 3
.endif

# This target has debugging value only, really.
makeflist: .PHONY check_DESTDIR
	${SETSENV} ${HOST_SH} ${.CURDIR}/makeflist

#
# SET BUILDING TARGETS
#

.if ${MKUNPRIVED} == "no"
METALOG.unpriv=
.else
METALOG.unpriv=	-M ${METALOG}
.endif

.PRECIOUS: checkflist
checkflist: check_DESTDIR
	${SETSENV} ${HOST_SH} ${.CURDIR}/checkflist ${CHECKFLIST_FLAGS} \
	    ${METALOG.unpriv}

.PRECIOUS: checkflist-x11
checkflist-x11: check_DESTDIR
	${SETSENV} ${HOST_SH} ${.CURDIR}/checkflist -x ${CHECKFLIST_FLAGS}

.PRECIOUS: maketars
maketars: check_DESTDIR check_RELEASEDIR .WAIT maketarsetup .WAIT ${MAKETARSETS:@.TARS.@${RELEASEDIR}/${MACHINE}/binary/sets/${.TARS.}.tgz@}
	@true

maketarsetup: .PHONY
.if defined(DESTDIR) && ${DESTDIR} != ""
	${MAKE} checkflist
.endif
	mkdir -p ${RELEASEDIR}/${MACHINE}/binary/sets
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		rm -f ${RELEASEDIR}/${MACHINE}/binary/sets/$$i.tmp; done
	
.for tar in ${MAKETARSETS}
${RELEASEDIR}/${MACHINE}/binary/sets/${tar}.tgz:
	${SETSENV} ${HOST_SH} ${.CURDIR}/maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.unpriv} -N ${NETBSDSRCDIR}/etc \
	    -t ${RELEASEDIR}/${MACHINE}/binary/sets ${tar}
.endfor

.PRECIOUS: makesrctars
makesrctars: check_RELEASEDIR
	mkdir -p ${RELEASEDIR}/source/sets
	${SETSENV} ${HOST_SH} ${.CURDIR}/makesrctars \
	    ${NETBSDSRCDIR} ${RELEASEDIR}/source/sets

.PRECIOUS: makesums
makesums: check_RELEASEDIR .WAIT ${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		mv ${RELEASEDIR}/${MACHINE}/binary/sets/$$i.tmp \
			${RELEASEDIR}/${MACHINE}/binary/sets/$$i; done

.for tar in ${MAKETARSETS}
do-sum-${tar}: .PHONY ${RELEASEDIR}/${MACHINE}/binary/sets/${tar}.tgz
	${SETSENV} ${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/binary/sets ${tar}.tgz
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		${TOOL_CAT} ${RELEASEDIR}/${MACHINE}/binary/sets/$$i >> ${RELEASEDIR}/${MACHINE}/binary/sets/$$i.tmp; \
	done
.endfor

.PRECIOUS: installsets
installsets: check_DESTDIR
.if !defined(INSTALLDIR)
	@echo "setenv INSTALLDIR before doing that!"
	@false
.endif
	${SETSENV} ${HOST_SH} ${.CURDIR}/maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.unpriv} -N ${NETBSDSRCDIR}/etc \
	    -i ${INSTALLDIR} ${INSTALLSETS}

#
# MAIN ENTRY POINTS
#

sets: maketars .WAIT makesums
	@true

sourcesets: makesrctars
	@true

.include <bsd.sys.mk>
