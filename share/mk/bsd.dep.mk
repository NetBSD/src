#	$NetBSD: bsd.dep.mk,v 1.73 2011/09/10 16:57:35 apb Exp $

##### Basic targets
realdepend:	beforedepend .depend afterdepend
.ORDER:		beforedepend .depend afterdepend

beforedepend .depend afterdepend: # ensure existence

##### Default values
MKDEP?=			mkdep
MKDEP_SUFFIXES?=	.o

##### Build rules
# some of the rules involve .h sources, so remove them from mkdep line

.if defined(SRCS)							# {
__acpp_flags=	${_ASM_TRADITIONAL_CPP}

__DPSRCS.all=	${SRCS:C/\.(c|m|s|S|C|cc|cpp|cxx)$/.d/} \
		${DPSRCS:C/\.(c|m|s|S|C|cc|cpp|cxx)$/.d/}
__DPSRCS.d=	${__DPSRCS.all:O:u:M*.d}
__DPSRCS.notd=	${__DPSRCS.all:O:u:N*.d}

.NOPATH: .depend ${__DPSRCS.d}

.if !empty(__DPSRCS.d)							# {
${__DPSRCS.d}: ${__DPSRCS.notd} ${DPSRCS}
.endif									# }

.depend: ${__DPSRCS.d}
	${_MKTARGET_CREATE}
	rm -f .depend
	${MKDEP} -d -f ${.TARGET} -s ${MKDEP_SUFFIXES:Q} ${__DPSRCS.d}

.SUFFIXES: .d .s .S .c .C .cc .cpp .cxx .m

.c.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEPFLAGS} \
	    ${CFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.m.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEPFLAGS} \
	    ${OBJCFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.s.d .S.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEPFLAGS} \
	    ${AFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${__acpp_flags} ${.IMPSRC}

.C.d .cc.d .cpp.d .cxx.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEPFLAGS} \
	    ${CXXFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.endif # defined(SRCS)							# }

##### Clean rules
.if defined(SRCS)
CLEANDIRFILES+= .depend ${__DPSRCS.d} ${.CURDIR}/tags ${CLEANDEPEND}
.endif

##### Custom rules
.if !target(tags)
tags: ${SRCS}
.if defined(SRCS)
	-cd "${.CURDIR}"; ctags -f /dev/stdout ${.ALLSRC:N*.h} | \
	    ${TOOL_SED} "s;\${.CURDIR}/;;" > tags
.endif
.endif

##### Pull in related .mk logic
.include <bsd.clean.mk>
