#	$Id: bsd.dep.mk,v 1.1 1993/08/15 19:37:04 mycroft Exp $

# some of the rules involve .h sources, so remove them from mkdep line
.if !target(depend)
depend: beforedepend .depend afterdepend
.if defined(SRCS)
.depend: ${SRCS}
	rm -f .depend
	files="${.ALLSRC:M*.c}"; \
	if [ "$$files" != "" ]; then \
	  mkdep -a ${MKDEP} ${CFLAGS:M-[ID]*} $$files; \
	fi
	files="${.ALLSRC:M*.cc} ${.ALLSRC:M*.C} ${.ALLSRC:M*.cxx}"; \
	if [ "$$files" != "  " ]; then \
	  mkdep -a ${MKDEP} -+ ${CXXFLAGS:M-[ID]*} $$files; \
	fi
.endif
.endif
