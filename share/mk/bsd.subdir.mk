#	$NetBSD: bsd.subdir.mk,v 1.13 1997/03/24 21:54:22 christos Exp $
#	@(#)bsd.subdir.mk	5.9 (Berkeley) 2/1/91

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
	echo "===> $${_nextdir_}"; \
	cd ${.CURDIR}/$${_newdir_}; \
	${MAKE} _THISDIR_="$${_nextdir_}" $${target});

.for dir in ${SUBDIR}
all-${dir}: __SUBDIRINTERNALUSE
install-${dir}: __SUBDIRINTERNALUSE
realinstall-${dir}: __SUBDIRINTERNALUSE
clean-${dir}: __SUBDIRINTERNALUSE
cleandir-${dir}: __SUBDIRINTERNALUSE
includes-${dir}: __SUBDIRINTERNALUSE
depend-${dir}: __SUBDIRINTERNALUSE
lint-${dir}: __SUBDIRINTERNALUSE
obj-${dir}: __SUBDIRINTERNALUSE
tags-${dir}: __SUBDIRINTERNALUSE

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
