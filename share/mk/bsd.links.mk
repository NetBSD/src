#	$NetBSD: bsd.links.mk,v 1.6 1997/05/09 13:25:55 mycroft Exp $

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
		echo ".if !defined(BUILD)"; \
		echo "$$t: .MADE"; \
		echo ".endif"; \
		echo ".if !defined(UPDATE)"; \
		echo ".PHONY: $$t"; \
		echo ".endif"; \
		echo ".PRECIOUS: $$t"; \
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
		echo ".PRECIOUS: $$t"; \
		echo "$$t: $$l"; \
		echo "	@echo \"$$t -> $$l\""; \
		echo "	@rm -f $$t"; \
		echo "	@ln $$l $$t"; \
	done | make -f -
.endif

.if !target(linksinstall)
linksinstall:
.endif
