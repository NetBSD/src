#	$Id: bsd.dep.mk,v 1.8 1994/06/30 05:21:28 cgd Exp $

# some of the rules involve .h sources, so remove them from mkdep line
.if !target(depend)
depend: beforedepend .depend _SUBDIRUSE afterdepend
.if defined(SRCS)
.depend: ${SRCS}
	rm -f .depend
	files="${.ALLSRC:M*.s} ${.ALLSRC:M*.S}"; \
	if [ "$$files" != " " ]; then \
	  mkdep -a ${MKDEP} ${CFLAGS:M-[ID]*} ${AINC} $$files; \
	fi
	files="${.ALLSRC:M*.c}"; \
	if [ "$$files" != "" ]; then \
	  mkdep -a ${MKDEP} ${CFLAGS:M-[ID]*} $$files; \
	fi
	files="${.ALLSRC:M*.cc} ${.ALLSRC:M*.C} ${.ALLSRC:M*.cxx}"; \
	if [ "$$files" != "  " ]; then \
	  mkdep -a ${MKDEP} -+ ${CXXFLAGS:M-[ID]*} $$files; \
	fi
.else
.depend:
.endif
.if !target(beforedepend)
beforedepend:
.endif
.if !target(afterdepend)
afterdepend:
.endif
.endif

.if !target(tags)
.if defined(SRCS)
tags: ${SRCS} _SUBDIRUSE
	-cd ${.CURDIR}; ctags -f /dev/stdout ${.ALLSRC:N*.h} | \
	    sed "s;\${.CURDIR}/;;" > tags
.else
tags:
.endif
.endif

.if defined(SRCS)
cleandir: cleandepend
cleandepend:
	rm -f .depend ${.CURDIR}/tags
.endif
