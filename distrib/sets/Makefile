#	$NetBSD: Makefile,v 1.29 2003/08/04 05:53:20 mrg Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SETSENV=	DESTDIR=${DESTDIR:Q} \
		MACHINE=${MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
		CKSUM=${TOOL_CKSUM:Q} MAKE=${MAKE:Q} MTREE=${TOOL_MTREE:Q} \
		PAX=${TOOL_PAX:Q} 

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
	${SETSENV} ${HOST_SH} ${.CURDIR}/checkflist ${METALOG.unpriv}

.PRECIOUS: checkflist-x11
checkflist-x11: check_DESTDIR
	${SETSENV} ${HOST_SH} ${.CURDIR}/checkflist -x11

.PRECIOUS: maketars
maketars: check_DESTDIR check_RELEASEDIR
.if defined(DESTDIR) && ${DESTDIR} != ""
	${MAKE} checkflist${CHECKFLISTFLAGS}
.endif
	mkdir -p ${RELEASEDIR}/${MACHINE}/binary/sets
	${SETSENV} ${HOST_SH} ${.CURDIR}/maketars -d ${DESTDIR:S,^$,/,} \
	    ${METALOG.unpriv} -N ${NETBSDSRCDIR}/etc \
	    -t ${RELEASEDIR}/${MACHINE}/binary/sets ${MAKETARSETS}

.PRECIOUS: makesrctars
makesrctars: check_RELEASEDIR
	mkdir -p ${RELEASEDIR}/source/sets
	${SETSENV} ${HOST_SH} ${.CURDIR}/makesrctars \
	    ${NETBSDSRCDIR} ${RELEASEDIR}/source/sets

.PRECIOUS: makesums
makesums: check_RELEASEDIR
	${SETSENV} ${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/binary/sets

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
