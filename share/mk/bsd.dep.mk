#	$NetBSD: bsd.dep.mk,v 1.49 2003/07/28 15:07:16 lukem Exp $

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

DEPENDSRCS.src=	${SRCS:M*.c}	${DPSRCS:M*.c}		\
		${SRCS:M*.m}	${DPSRCS:M*.m}		\
		${SRCS:M*.[sS]}	${DPSRCS:M*.[sS]}	\
		${SRCS:M*.C}	${DPSRCS:M*.C}		\
		${SRCS:M*.cc}	${DPSRCS:M*.cc}		\
		${SRCS:M*.cpp}	${DPSRCS:M*.cpp}	\
		${SRCS:M*.cxx}	${DPSRCS:M*.cxx}
DEPENDSRCS.dep=	${DEPENDSRCS.src:C/(.*)/\1.dep/g:S/^.dep$//}
DEPENDSRCS=	.depend ${DEPENDSRCS.dep}

.depend: ${SRCS} ${DPSRCS} ${DEPENDSRCS.dep}
	@rm -f .depend
	cat ${DEPENDSRCS.dep} > .depend

.SUFFIXES: .c .c.dep .m .m.m.dep .s .s.s.dep .S .S.S.dep
.SUFFIXES: .C .C.C.dep .cc .cc.cc.dep .cpp .cpp.cpp.dep .cxx .cxx.cxx.dep

.c.c.dep:
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${CFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.m.m.dep:
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${OBJCFLAGS:M-[ID]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.s.s.dep .S.S.dep:
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${AFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${__acpp_flags} ${AINC} ${.IMPSRC}

.C.C.dep .cc.cc.dep .cpp.cpp.dep .cxx.cxx.dep:
	${MKDEP} -a -f ${.TARGET} ${MKDEPFLAGS} ${CXXFLAGS:M-[ID]*} \
	    ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} \
	    ${DESTDIR}/usr/include/g++} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.endif # defined(SRCS)

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
