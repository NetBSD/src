#	$NetBSD: bsd.dep.mk,v 1.47 2003/07/27 14:49:22 mrg Exp $

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
.if empty(HOST_CYGWIN)
.depend: ${SRCS} ${DPSRCS}
	@rm -f .depend
	@files="${.ALLSRC:M*.s} ${.ALLSRC:M*.S}"; \
	if [ "$$files" != " " ]; then \
	  echo ${MKDEP} -a ${MKDEPFLAGS} \
	    ${AFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} ${__acpp_flags} ${AINC:Q} \
	    $$files; \
	  ${MKDEP} -a ${MKDEPFLAGS} \
	    ${AFLAGS:M-[ID]*} ${CPPFLAGS} ${__acpp_flags} ${AINC} $$files; \
	fi
	@files="${.ALLSRC:M*.c}"; \
	if [ "$$files" != "" ]; then \
	  echo ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} $$files; \
	  ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CFLAGS:M-[ID]*} ${CPPFLAGS} $$files; \
	fi
	@files="${.ALLSRC:M*.m}"; \
	if [ "$$files" != "" ]; then \
	  echo ${MKDEP} -a ${MKDEPFLAGS} \
	    ${OBJCFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} $$files; \
	  ${MKDEP} -a ${MKDEPFLAGS} \
	    ${OBJCFLAGS:M-[ID]*} ${CPPFLAGS} $$files; \
	fi
	@files="${.ALLSRC:M*.cc} ${.ALLSRC:M*.C} ${.ALLSRC:M*.cxx} ${.ALLSRC:M*.cpp}"; \
	if [ "$$files" != "   " ]; then \
	  echo ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} $$files; \
	  ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-[ID]*} ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} ${DESTDIR}/usr/include/g++} ${CPPFLAGS} $$files; \
	fi
.else
#
# Cygwin workarounds for limited environment & command line space
#

SRCS_S = ${SRCS:M*.[sS]} ${DPSRCS:M*.[sS]}
SRCS_C = ${SRCS:M*.c} ${DPSRCS:M*.c}
SRCS_M = ${SRCS:M*.m} ${DPSRCS:M*.m}
SRCS_X = ${SRCS:M*.C} ${DPSRCS:M*.C} \
         ${SRCS:M*.cc} ${DPSRCS:M*.cc} \
         ${SRCS:M*.cpp} ${DPSRCS:M*.cpp} \
	 ${SRCS:M*.cxx} ${DPSRCS:M*.cxx}

.depend: ${SRCS} ${DPSRCS} \
	 ${SRCS_S:C/(.*)/\1.dep/g:S/^.dep$//g} \
	 ${SRCS_C:C/(.*)/\1.dep/g:S/^.dep$//g} \
	 ${SRCS_M:C/(.*)/\1.dep/g:S/^.dep$//g} \
	 ${SRCS_X:C/(.*)/\1.dep/g:S/^.dep$//g}
	@rm -f .depend
	@cat ${.ALLSRC:M*.dep} > .depend

.for F in ${SRCS_S:O:u}
.NOPATH: ${F:C/(.*)/\1.dep/g}
${F:C/(.*)/\1.dep/g}: ${F}
	@echo ${MKDEP} -a -f $@ ${MKDEPFLAGS} \
	    ${AFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} ${__acpp_flags} ${AINC:Q} \
	    ${.ALLSRC}
	@${MKDEP} -a -f $@ ${MKDEPFLAGS} ${AFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${__acpp_flags} ${AINC} ${.ALLSRC}
.endfor

.for F in ${SRCS_C:O:u}
.NOPATH: ${F:C/(.*)/\1.dep/g}
${F:C/(.*)/\1.dep/g}: ${F}
	@echo ${MKDEP} -a -f $@ ${MKDEPFLAGS} ${CFLAGS:M-[ID]*:Q} \
	    ${CPPFLAGS:Q} ${.ALLSRC}
	@${MKDEP} -a -f $@ ${MKDEPFLAGS} ${CFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${.ALLSRC}
.endfor

.for F in ${SRCS_M:O:u}
.NOPATH: ${F:C/(.*)/\1.dep/g}
${F:C/(.*)/\1.dep/g}: ${F}
	@echo ${MKDEP} -a -f $@ ${MKDEPFLAGS} ${OBJCFLAGS:M-[ID]*:Q} \
	    ${CPPFLAGS:Q} ${.ALLSRC}
	@${MKDEP} -a -f $@ ${MKDEPFLAGS} ${OBJCFLAGS:M-[ID]*} ${CPPFLAGS} \
	    ${.ALLSRC}
.endfor

.for F in ${SRCS_X:O:u}
.NOPATH: ${F:C/(.*)/\1.dep/g}
${F:C/(.*)/\1.dep/g}: ${F}
	@echo ${MKDEP} -a -f $@ ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} ${.ALLSRC}
	@${MKDEP} -a -f $@ ${MKDEPFLAGS} ${CXXFLAGS:M-[ID]*} \
	    ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEMXX} \
	    ${DESTDIR}/usr/include/g++} ${CPPFLAGS} ${.ALLSRC}
.endfor

.endif # Cygwin
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
