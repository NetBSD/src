#	$NetBSD: bsd.links.mk,v 1.23 2003/07/20 14:53:10 lukem Exp $

.include <bsd.init.mk>

##### Basic targets
.PHONY:		linksinstall
install:	linksinstall

##### Default values
LINKS?=
SYMLINKS?=

##### Install rules
linksinstall::	realinstall
.if !empty(SYMLINKS)
	@(set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; shift; \
		t=${DESTDIR}$$1; shift; \
		if  ttarg=`${TOOL_STAT} -qf '%Y' $$t` && \
		    [ "$$l" = "$$ttarg" ]; then \
			continue ; \
		fi ; \
		echo "$$t -> $$l"; \
		${INSTALL_SYMLINK} ${SYSPKGTAG} $$l $$t; \
	 done; )
.endif
.if !empty(LINKS)
	@(set ${LINKS}; \
	 while test $$# -ge 2; do \
		l=${DESTDIR}$$1; shift; \
		t=${DESTDIR}$$1; shift; \
		if  ldevino=`${TOOL_STAT} -qf '%d %i' $$l` && \
		    tdevino=`${TOOL_STAT} -qf '%d %i' $$t` && \
		    [ "$$ldevino" = "$$tdevino" ]; then \
			continue ; \
		fi ; \
		echo "$$t -> $$l"; \
		${INSTALL_LINK} ${SYSPKGTAG} $$l $$t; \
	done ; )
.endif
