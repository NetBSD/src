#	$NetBSD: Makefile,v 1.12 2002/04/13 12:38:31 lukem Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

SETSENV=	DESTDIR=${DESTDIR:Q} \
		MACHINE=${MACHINE:Q} MACHINE_ARCH=${MACHINE_ARCH:Q} \
		MAKE=${MAKE:Q} MTREE=${MTREE:Q} PAX=${PAX:Q} 

print_machine: .PHONY
	@echo ${MACHINE}

print_machine_arch: .PHONY
	@echo ${MACHINE_ARCH}

print_machine_cpu: .PHONY
	@echo ${MACHINE_CPU}

print_object_fmt: .PHONY
	@echo ${OBJECT_FMT}

print_use_new_toolchain: .PHONY
	@echo "${USE_NEW_TOOLCHAIN:Dyes}"

print_x11_version: .PHONY
.if defined(USE_XF86_4) && (${USE_XF86_4} != no)
	@echo 4
.else
	@echo 3
.endif

# This target has debugging value only, really.
makeflist: .PHONY check_DESTDIR
	${SETSENV} sh ${.CURDIR}/makeflist

#
# SET BUILDING TARGETS
#

.PRECIOUS: checkflist
checkflist: check_DESTDIR
	${SETSENV} sh ${.CURDIR}/checkflist

.PRECIOUS: checkflist-x11
checkflist-x11: check_DESTDIR
	${SETSENV} sh ${.CURDIR}/checkflist -x11

.PRECIOUS: maketars
maketars: check_DESTDIR check_RELEASEDIR
	${SETSENV} sh ${.CURDIR}/maketars ${UNPRIVED:D-M ${METALOG}} \
		-d ${DESTDIR} -t ${RELEASEDIR}/binary/sets ${MAKETARSETS}

.PRECIOUS: makesums
makesums: check_RELEASEDIR
	${SETSENV} ${MAKESUMS} -t ${RELEASEDIR}/binary/sets

#
# MAIN ENTRY POINTS
#

sets: maketars makesums
	@true

.include <bsd.sys.mk>
