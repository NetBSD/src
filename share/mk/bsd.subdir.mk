#	$NetBSD: bsd.subdir.mk,v 1.18 1997/05/06 23:53:40 mycroft Exp $
#	@(#)bsd.subdir.mk	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>

.if !target(.MAIN)
.MAIN: all
.endif

_SUBDIRUSE: .USE ${SUBDIR:S/^/${.TARGET}-/}

__SUBDIRINTERNALUSE: .USE
	@(_maketarget_="${.TARGET:S/realinstall/install/}"; \
	entry="$${_maketarget_#*-}";\
	target="$${_maketarget_%%-*}";\
	set -e; if test -d ${.CURDIR}/$${entry}.${MACHINE}; then \
		_newdir_="$${entry}.${MACHINE}"; \
	else \
		_newdir_="$${entry}"; \
	fi; \
	if test X"${_THISDIR_}" = X""; then \
		_nextdir_="$${_newdir_}"; \
	else \
		_nextdir_="$${_THISDIR_}/$${_newdir_}"; \
	fi; \
	if test -d ${.CURDIR}/$${_newdir_}; then \
		echo "===> $${_nextdir_}"; \
		cd ${.CURDIR}/$${_newdir_}; \
		${MAKE} _THISDIR_="$${_nextdir_}" $${target}; \
	else \
		echo "===> $${_nextdir_} [skipped: missing]"; \
	fi)

.for dir in ${SUBDIR}
.for targ in ${TARGETS}
.PHONY: ${targ}-${dir}
${targ}-${dir}: .MAKE __SUBDIRINTERNALUSE
.endfor

# Backward-compatibility with the old rules.  If this went away,
# 'xlint' could become 'lint', 'xinstall' could become 'install', etc.
${dir}: all-${dir}
.endfor

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif
.if !target(afterinstall)
afterinstall:
.endif
install: ${MANINSTALL}
${MANINSTALL}: afterinstall
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

.if !target(includes)
includes: _SUBDIRUSE
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

.include <bsd.own.mk>
