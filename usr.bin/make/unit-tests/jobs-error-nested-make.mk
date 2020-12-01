# $NetBSD: jobs-error-nested-make.mk,v 1.1 2020/12/01 17:50:04 rillig Exp $
#
# Ensure that in jobs mode, when a command fails, the current directory is
# printed, to aid in debugging, even if the target is marked as .MAKE.
# This marker is typically used for targets like 'all' that descend into
# subdirectories.
#
# XXX: In case of .MAKE targets, the "stopped if" output has been suppressed
# since job.c 1.198 from 2020-06-19.
#
# https://gnats.netbsd.org/55578
# https://gnats.netbsd.org/55832

.MAKEFLAGS: -j1

all: .PHONY .MAKE
	${MAKE} -f ${MAKEFILE} nested

nested: .PHONY .MAKE
	false
