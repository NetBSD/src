#	$NetBSD: bsd.links.mk,v 1.11 2000/02/10 18:18:02 drochner Exp $

.PHONY:		linksinstall
realinstall:	linksinstall

.if defined(SYMLINKS) && !empty(SYMLINKS)
linksinstall::
	@(set ${SYMLINKS}; \
	 echo ".include <bsd.own.mk>"; \
	 while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo "realall: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln -s $$l $$t"; \
	 done; \
	) | ${MAKE} -f- all
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
		echo "	@rm -f $$t"; \
		echo "	@ln $$l $$t"; \
	 done; \
	) | ${MAKE} -f- all
.endif

.if !target(linksinstall)
linksinstall:
.endif
