#	$NetBSD: bsd.subdir.mk,v 1.33 2000/04/10 14:47:23 mrg Exp $
#	@(#)bsd.subdir.mk	8.1 (Berkeley) 6/8/93

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.MAIN:		all
.endif

.for dir in ${SUBDIR}
.if exists(${dir}.${MACHINE})
__REALSUBDIR+=${dir}.${MACHINE}
.else
__REALSUBDIR+=${dir}
.endif
.endfor

.if defined(DESTDIR) && !defined(_USE_INSTALLED_MK)
_M=-m ${DESTDIR}/usr/share/mk
.else
_M=
.endif

.for dir in ${__REALSUBDIR}
.for targ in ${TARGETS}
.PHONY: ${targ}-${dir}
${targ}-${dir}: .MAKE
	@case "${dir}" in /*) \
		echo "${targ} ===> ${dir}"; \
		cd ${dir}; \
		${MAKE} ${_M} "_THISDIR_=${dir}/" ${targ}; \
		;; \
	*) \
		echo "${targ} ===> ${_THISDIR_}${dir}"; \
		cd ${.CURDIR}/${dir}; \
		${MAKE} ${_M} "_THISDIR_=${_THISDIR_}${dir}/" ${targ}; \
		;; \
	esac
subdir-${targ}: ${targ}-${dir}
${targ}: subdir-${targ}
.endfor
.endfor

# Make sure all of the standard targets are defined, even if they do nothing.
${TARGETS}:
