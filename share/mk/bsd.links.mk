#	$NetBSD: bsd.links.mk,v 1.15 2001/11/02 05:21:51 tv Exp $

##### Basic targets
.PHONY:		linksinstall
realinstall:	linksinstall

##### Default values
LINKS?=
SYMLINKS?=

##### Install rules
linksinstall::
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
		rm -rf $$t; ln -s $$l $$t; \
	 done; )
.endif
.if !empty(LINKS)
	@(set ${LINKS}; \
	 echo ".include <bsd.own.mk>"; \
	 while test $$# -ge 2; do \
		l=${DESTDIR}$$1; shift; \
		t=${DESTDIR}$$1; shift; \
		echo "realall: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t; ln $$l $$t"; \
	 done; \
	) | ${MAKE} -f- all
.endif
