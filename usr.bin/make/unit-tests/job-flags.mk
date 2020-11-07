# $NetBSD: job-flags.mk,v 1.1 2020/11/07 20:01:17 rillig Exp $
#
# Tests for Job.flags, which are controlled by special source dependencies
# like .SILENT or .IGNORE, as well as the command line options -s or -i.

.MAKEFLAGS: -j1

all: silent .WAIT ignore

.BEGIN:
	@echo $@

silent: .SILENT
	echo $@

ignore: .IGNORE
	@echo $@
	true in $@
	false in $@
	@echo 'Still there in $@'

.END:
	@echo $@
