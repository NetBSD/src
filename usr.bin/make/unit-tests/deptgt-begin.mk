# $NetBSD: deptgt-begin.mk,v 1.4 2020/11/15 20:47:01 rillig Exp $
#
# Tests for the special target .BEGIN in dependency declarations,
# which is a container for commands that are run before any other
# commands from the shell lines.

.BEGIN:
	: $@

# To register a custom action to be run at the beginning, the simplest way is
# to directly place some commands on the '.BEGIN' target.  This doesn't scale
# though, since the ':' dependency operator prevents that any other place may
# add its commands after this.
#
# There are several ways to resolve this situation, which are detailed below.
.BEGIN:
	: Making another $@.

# One way to run commands at the beginning is to define a custom target and
# make the .BEGIN depend on that target.  This way, the commands from the
# custom target are run even before the .BEGIN target.
.BEGIN: before-begin
before-begin: .PHONY .NOTMAIN
	: Making $@ before .BEGIN.

# Another way is to define a custom target and make that a .USE dependency.
# This way, its commands are appended to the commands of the .BEGIN target
# just before the .BEGIN target is made.
#
# XXX: For some reason, the commands from the .USE target are not run.
# XXX: .USE nodes should not be candidates for the .MAIN node.
.BEGIN: use
use: .USE .NOTMAIN
	: Making $@ from a .USE dependency.

# Same as with .USE, but run the commands before the main commands from the
# .BEGIN target.
#
# XXX: For some reason, the commands from the .USEBEFORE target are not run.
# XXX: .USEBEFORE nodes should not be candidates for the .MAIN node.
.BEGIN: use-before
use-before: .USEBEFORE .NOTMAIN
	: Making $@ from a .USEBEFORE dependency.

all:
	: $@

_!=	echo : parse time 1>&2
