#	$NetBSD: Makefile,v 1.5 2002/02/11 16:23:17 tron Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

#
# DETERMINE WHICH CRYPTO SET TO USE
#

.include <bsd.own.mk>

#
# UTILITY TARGETS
#

check_destdir: .PHONY
.if !defined(DESTDIR)
	@echo "DESTDIR must be set"
	@false
.else
	@true
.endif

check_releasedir: .PHONY
.if !defined(RELEASEDIR)
	@echo "RELEASEDIR must be set"
	@false
.else
	@true
.endif

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
makeflist: .PHONY check_destdir
	DESTDIR=${DESTDIR} MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH} \
	    MAKE=${MAKE} sh ${.CURDIR}/makeflist

#
# SET BUILDING TARGETS
#

.PRECIOUS: checkflist
checkflist: check_destdir
	DESTDIR=${DESTDIR} MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH} \
	    MAKE=${MAKE} sh ${.CURDIR}/checkflist

.PRECIOUS: checkflist
checkflist-x11: check_destdir
	DESTDIR=${DESTDIR} MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH} \
	    MAKE=${MAKE} sh ${.CURDIR}/checkflist -x11

.PRECIOUS: maketars
maketars: check_destdir check_releasedir
	sh ${.CURDIR}/maketars -d ${DESTDIR} -t ${RELEASEDIR}/binary/sets

.PRECIOUS: makesums
makesums: check_releasedir
	sh ${.CURDIR}/makesums -t ${RELEASEDIR}/binary/sets

#
# MAIN ENTRY POINTS
#

sets: maketars makesums
	@true
