#	$NetBSD: bsd.links.mk,v 1.14 2000/07/07 04:35:36 cgd Exp $

.PHONY:		linksinstall
realinstall:	linksinstall

.if defined(SYMLINKS) && !empty(SYMLINKS)
linksinstall::
	@(set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
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
.if defined(LINKS) && !empty(LINKS)
linksinstall::
	@(set ${LINKS}; \
	 echo ".include <bsd.own.mk>"; \
	 while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo "realall: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t; ln $$l $$t"; \
	 done; \
	) | ${MAKE} -f- all
.endif

.if !target(linksinstall)
linksinstall:
.endif
