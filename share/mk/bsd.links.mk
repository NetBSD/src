#	$NetBSD: bsd.links.mk,v 1.3 1997/05/06 20:54:36 mycroft Exp $

.PHONY:		linksinstall

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
		echo "$$t:"; \
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
