#	$NetBSD: bsd.links.mk,v 1.1 1997/03/24 21:54:18 christos Exp $

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
		echo ".if !defined(BUILD)"; \
		echo "$$t: .MADE"; \
		echo ".endif"; \
		echo ".if !defined(UPDATE)"; \
		echo ".PHONY: $$t"; \
		echo ".endif"; \
		echo "$$t: $$l"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln -s $$l $$t"; \
	done | make -f -
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
		echo ".if !defined(BUILD)"; \
		echo "$$t: .MADE"; \
		echo ".endif"; \
		echo ".if !defined(UPDATE)"; \
		echo ".PHONY: $$t"; \
		echo ".endif"; \
		echo "$$t: $$l"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln $$l $$t"; \
	done | make -f -
.endif

.if !target(linksinstall)
linksinstall:
.endif
