#	$NetBSD: bsd.links.mk,v 1.8 1998/12/31 15:28:52 castor Exp $

.PHONY:		linksinstall
realinstall:	linksinstall

.if defined(SYMLINKS) && !empty(SYMLINKS)
linksinstall::
	@set ${SYMLINKS}; \
	 while test $$# -ge 2; do \
		l=$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo ".include <bsd.own.mk>"; \
		echo "all:: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln -s $$l $$t"; \
	done | ${MAKE} -f-
.endif
.if defined(LINKS) && !empty(LINKS)
linksinstall::
	@set ${LINKS}; \
	 while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo ".include <bsd.own.mk>"; \
		echo "all:: $$t"; \
		echo ".PHONY: $$t"; \
		echo "$$t:"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln $$l $$t"; \
	done | ${MAKE} -f-
.endif

.if !target(linksinstall)
linksinstall:
.endif
