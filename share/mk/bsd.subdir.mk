#	$NetBSD: bsd.subdir.mk,v 1.7 1994/06/30 05:31:23 cgd Exp $
#	@(#)bsd.subdir.mk	5.9 (Berkeley) 2/1/91

.if !target(.MAIN)
.MAIN: all
.endif

_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		(if test -d ${.CURDIR}/$${entry}.${MACHINE}; then \
			echo "===> $${entry}.${MACHINE}"; \
			cd ${.CURDIR}/$${entry}.${MACHINE}; \
		else \
			echo "===> $$entry"; \
			cd ${.CURDIR}/$${entry}; \
		fi; \
		${MAKE} ${.TARGET:S/realinstall/install/:S/.depend/depend/}); \
	done

${SUBDIR}::
	@if test -d ${.TARGET}.${MACHINE}; then \
		cd ${.CURDIR}/${.TARGET}.${MACHINE}; \
	else \
		cd ${.CURDIR}/${.TARGET}; \
	fi; \
	${MAKE} all
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif
.if !target(afterinstall)
afterinstall:
.endif
install: maninstall
maninstall: afterinstall
afterinstall: realinstall
realinstall: beforeinstall _SUBDIRUSE
.endif

.if !target(all)
all: _SUBDIRUSE
.endif

.if !target(clean)
clean: _SUBDIRUSE
.endif

.if !target(cleandir)
cleandir: _SUBDIRUSE
.endif

.if !target(depend)
depend: _SUBDIRUSE
.endif

.if !target(lint)
lint: _SUBDIRUSE
.endif

.if !target(obj)
obj: _SUBDIRUSE
.endif

.if !target(tags)
tags: _SUBDIRUSE
.endif
