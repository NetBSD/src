#	from: @(#)bsd.subdir.mk	5.9 (Berkeley) 2/1/91
#	$Id: bsd.subdir.mk,v 1.4 1993/08/17 17:00:40 mycroft Exp $

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
install: maninstall _SUBDIRUSE
maninstall: afterinstall
afterinstall: realinstall
realinstall: beforeinstall
.endif

all clean cleandir depend lint obj tags: _SUBDIRUSE
