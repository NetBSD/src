#	$NetBSD: bsd.dep.mk,v 1.53 2003/07/31 13:47:32 lukem Exp $

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

.if defined(SRCS)							# {
__acpp_flags=	-traditional-cpp

DEPENDSRCS.src=	${SRCS:M*.c}	${DPSRCS:M*.c}		\
		${SRCS:M*.m}	${DPSRCS:M*.m}		\
		${SRCS:M*.[sS]}	${DPSRCS:M*.[sS]}	\
		${SRCS:M*.C}	${DPSRCS:M*.C}		\
		${SRCS:M*.cc}	${DPSRCS:M*.cc}		\
		${SRCS:M*.cpp}	${DPSRCS:M*.cpp}	\
		${SRCS:M*.cxx}	${DPSRCS:M*.cxx}
DEPENDSRCS.d=	${DEPENDSRCS.src:R:S/$/.d/g}
DEPENDSRCS=	.depend ${DEPENDSRCS.d}

${DEPENDSRCS.d}: ${SRCS} ${DPSRCS}

.depend: ${DEPENDSRCS.d}
	@rm -f .depend
	cat ${DEPENDSRCS.d} > .depend

.NOPATH: ${DEPENDSRCS}

.SUFFIXES: .d .c .m .s .S .C .cc .cpp .cxx

.c.d:
	${MKDEP} -f ${.TARGET} ${MKDEPFLAGS} ${CFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.m.d:
	${MKDEP} -f ${.TARGET} ${MKDEPFLAGS} ${OBJCFLAGS:M-[ID]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.s.d .S.d:
	${MKDEP} -f ${.TARGET} ${MKDEPFLAGS} ${AFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${__acpp_flags} ${AINC} ${.IMPSRC}

.C.d .cc.d .cpp.d .cxx.d:
	${MKDEP} -f ${.TARGET} ${MKDEPFLAGS} ${CXXFLAGS:M-[ID]*} \
	    ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} \
	    ${DESTDIR}/usr/include/g++} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.endif # defined(SRCS)							# }

##### Clean rules
cleandepend:
.if defined(SRCS)
	rm -f ${DEPENDSRCS} ${.CURDIR}/tags ${CLEANDEPEND}
.endif

##### Custom rules
.if !target(tags)
tags: ${SRCS}
.if defined(SRCS)
	-cd ${.CURDIR}; ctags -f /dev/stdout ${.ALLSRC:N*.h} | \
	    sed "s;\${.CURDIR}/;;" > tags
.endif
.endif
