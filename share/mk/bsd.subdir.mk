#	$NetBSD: bsd.subdir.mk,v 1.26 1997/10/11 07:26:54 mycroft Exp $
#	@(#)bsd.subdir.mk	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>

.if !target(.MAIN)
.MAIN: all
.endif

.for dir in ${SUBDIR}
.if exists(${dir}.${MACHINE})
__REALSUBDIR+=${dir}.${MACHINE}
.else
__REALSUBDIR+=${dir}
.endif
.endfor

.for dir in ${__REALSUBDIR}
.for targ in ${TARGETS}
.PHONY: ${targ}-${dir}
${targ}-${dir}: .MAKE
	@echo "===> ${_THISDIR_}${dir}"
	@cd ${.CURDIR}/${dir}; \
	${MAKE} "_THISDIR_=${_THISDIR_}${dir}/" ${targ}
${targ}: ${targ}-${dir}
.endfor

# Backward-compatibility with the old rules.  If this went away,
# 'xlint' could become 'lint', 'xinstall' could become 'install', etc.
${dir}: all-${dir}
.endfor

# Make sure all of the standard targets are defined, even if they do nothing.
${TARGETS}:
