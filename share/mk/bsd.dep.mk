#	$NetBSD: bsd.dep.mk,v 1.35 2002/06/04 21:22:54 thorpej Exp $

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
.if defined(HAVE_GCC3)
__acpp_flags=
.else
__acpp_flags=	-traditional-cpp
.endif
.NOPATH:	.depend
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
	@files="${.ALLSRC:M*.cc} ${.ALLSRC:M*.C} ${.ALLSRC:M*.cxx}"; \
	if [ "$$files" != "  " ]; then \
	  echo ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-[ID]*:Q} ${CPPFLAGS:Q} $$files; \
	  ${MKDEP} -a ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-[ID]*} ${DESTDIR:D-nostdinc++ ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include/g++} ${CPPFLAGS} $$files; \
	fi
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
