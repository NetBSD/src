#	$NetBSD: bsd.links.mk,v 1.21 2002/10/22 18:48:28 perry Exp $

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
		if [ -h $$t ]; then \
			cur=`ls -ld $$t | awk '{print $$NF}'` ; \
			if [ "$$cur" = "$$l" ]; then \
				continue ; \
			fi; \
		fi; \
		echo "$$t -> $$l"; \
		${INSTALL_SYMLINK} ${SYSPKGTAG} $$l $$t; \
	 done; )
.endif
.if !empty(LINKS)
	@(set ${LINKS}; \
	 echo ".include <bsd.own.mk>"; \
	 while test $$# -ge 2; do \
		l=${DESTDIR}$$1; shift; \
		t=${DESTDIR}$$1; shift; \
		echo "realall: $$t"; \
		echo "$$t!"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@${INSTALL_LINK} ${SYSPKGTAG} $$l $$t"; \
	 done; \
	) | ${MAKE} -f- all
.endif
