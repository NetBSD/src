#	$NetBSD: bsd.subdir.mk,v 1.38 2000/06/06 08:54:06 mycroft Exp $
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

.if defined(DESTDIR) && exists(${DESTDIR}/usr/share/mk/sys.mk)
_M=-m ${DESTDIR}/usr/share/mk
.else
_M=
.endif

__recurse: .USE
	@targ=${.TARGET:C/-.*$//};dir=${.TARGET:C/^[^-]*-//};		\
	case "$$dir" in /*)						\
		echo "$$targ ===> $$dir";				\
		cd "$$dir";						\
		${MAKE} ${_M} "_THISDIR_=$$dir/" $$targ;		\
		;;							\
	*)								\
		echo "$$targ ===> ${_THISDIR_}$$dir";			\
		cd "${.CURDIR}/$$dir";					\
		${MAKE} ${_M} "_THISDIR_=${_THISDIR_}$$dir/" $$targ;	\
		;;							\
	esac

.for targ in ${TARGETS}
.for dir in ${__REALSUBDIR}
.PHONY: ${targ}-${dir}
${targ}-${dir}: .MAKE __recurse
subdir-${targ}: ${targ}-${dir}
.endfor
.if defined(__REALSUBDIR)
${targ}: subdir-${targ}
.endif
.endfor

# Make sure all of the standard targets are defined, even if they do nothing.
${TARGETS}:
