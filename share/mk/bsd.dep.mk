#	$NetBSD: bsd.dep.mk,v 1.48 2003/07/28 08:59:52 lukem Exp $

##### Basic targets
.PHONY:		cleandepend
cleandir:	cleandepend
realdepend:	beforedepend .depend afterdepend
.ORDER:		beforedepend .depend afterdepend

beforedepend .depend afterdepend: # ensure existence

##### Default values
MKDEP?=		mkdep

##### Build rules
# some of the rules involve .h sources, so remove them from mkdep line

.if defined(SRCS)
__acpp_flags=	-traditional-cpp
.NOPATH:	.depend
SRCS_C=	${SRCS:M*.c} ${DPSRCS:M*.c}
DEPS_C=	${SRCS_C:C/(.*)/\1.dep/g:S/^.dep$//}
SRCS_M=	${SRCS:M*.m} ${DPSRCS:M*.m}
DEPS_M=	${SRCS_M:C/(.*)/\1.dep/g:S/^.dep$//}
SRCS_S=	${SRCS:M*.[sS]} ${DPSRCS:M*.[sS]}
DEPS_S=	${SRCS_S:C/(.*)/\1.dep/g:S/^.dep$//}
SRCS_X=	${SRCS:M*.C} ${DPSRCS:M*.C} \
	${SRCS:M*.cc} ${DPSRCS:M*.cc} \
	${SRCS:M*.cpp} ${DPSRCS:M*.cpp} \
	${SRCS:M*.cxx} ${DPSRCS:M*.cxx}
DEPS_X=	${SRCS_X:C/(.*)/\1.dep/g:S/^.dep$//}

CLEANDEPEND+=${DEPS_C} ${DEPS_M} ${DEPS_S} ${DEPS_X}

.depend: ${SRCS} ${DPSRCS} ${DEPS_C} ${DEPS_M} ${DEPS_S} ${DEPS_X}
	@rm -f .depend
	cat ${.ALLSRC:M*.dep} > .depend

.for F in ${DEPS_C:O:u}
.NOPATH: ${F}
${F}: ${F:R}
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${CFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.ALLSRC:T}} ${.ALLSRC}
.endfor

.for F in ${DEPS_M:O:u}
.NOPATH: ${F}
${F}: ${F:R}
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${OBJCFLAGS:M-[ID]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.ALLSRC:T}} ${.ALLSRC}
.endfor

.for F in ${DEPS_S:O:u}
.NOPATH: ${F}
${F}: ${F:R}
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${AFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.ALLSRC:T}} ${__acpp_flags} ${AINC} ${.ALLSRC}
.endfor

.for F in ${DEPS_X:O:u}
.NOPATH: ${F}
${F}: ${F:R}
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${CXXFLAGS:M-[ID]*} \
	    ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} \
	    ${DESTDIR}/usr/include/g++} \
	    ${CPPFLAGS} ${CPPFLAGS.${.ALLSRC:T}} ${.ALLSRC}
.endfor

.endif # defined(SRCS)

##### Clean rules
cleandepend:
.if defined(SRCS)
	rm -f .depend ${.CURDIR}/tags ${CLEANDEPEND}
.endif

##### Custom rules
.if !target(tags)
tags: ${SRCS}
.if defined(SRCS)
	-cd ${.CURDIR}; ctags -f /dev/stdout ${.ALLSRC:N*.h} | \
	    sed "s;\${.CURDIR}/;;" > tags
.endif
.endif
