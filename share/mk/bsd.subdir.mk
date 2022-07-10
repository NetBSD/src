#	$NetBSD: bsd.subdir.mk,v 1.56 2022/07/10 21:32:09 rillig Exp $
#	@(#)bsd.subdir.mk	8.1 (Berkeley) 6/8/93

.include <bsd.init.mk>

.if !defined(NOSUBDIR)					# {

.for dir in ${SUBDIR}
.if ${dir} != ".WAIT" && exists(${.CURDIR}/${dir}.${MACHINE})
__REALSUBDIR+=${dir}.${MACHINE}
.else
__REALSUBDIR+=${dir}
.endif
.endfor

.if ${MKGROFF} == "yes"
__REALSUBDIR+=${SUBDIR.roff}
.endif

__recurse: .USE
	@${MAKEDIRTARGET} ${.TARGET:C/^[^-]*-//} ${.TARGET:C/-.*$//}

.if make(cleandir)
__RECURSETARG=	${TARGETS:Nclean}
clean:
.else
__RECURSETARG=	${TARGETS}
.endif

.for targ in ${__RECURSETARG}
.for dir in ${__REALSUBDIR}
.if ${dir} == ".WAIT"
SUBDIR_${targ}+= .WAIT
.elif !commands(${targ}-${dir})
${targ}-${dir}: .PHONY .MAKE __recurse
SUBDIR_${targ}+= ${targ}-${dir}
.endif
.endfor
subdir-${targ}: .PHONY ${SUBDIR_${targ}}
${targ}: subdir-${targ}
.endfor

.endif	# ! NOSUBDIR					# }

${TARGETS}:	# ensure existence
