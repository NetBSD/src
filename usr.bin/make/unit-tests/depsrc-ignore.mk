# $NetBSD: depsrc-ignore.mk,v 1.3 2020/08/29 15:06:33 rillig Exp $
#
# Tests for the special source .IGNORE in dependency declarations,
# which ignores any command failures for that target.
#
# Even though ignore-errors fails, the all target is still made.
# Since the all target is not marked with .IGNORE, it stops at the
# first failing command.
#
# XXX: The messages in the output are confusing.
# The "ignored" comes much too late to be related to the "false
# ignore-errors".
# The "continuing" is confusing as well since it doesn't answer the
# question "continuing with what?".
#
# Even more interestingly, enabling the debugging option -de changes
# the order in which the messages appear.  Now the "ignored" message
# is issued in the correct position.  The manual page even defines the
# buffering of debug_file and stdout, so there should be no variance.

#.MAKEFLAGS: -de

all: ignore-errors

ignore-errors: .IGNORE
	@echo $@ begin
	false $@
	@echo $@ end

all:
	@echo $@ begin
	false $@
	@echo $@ end
