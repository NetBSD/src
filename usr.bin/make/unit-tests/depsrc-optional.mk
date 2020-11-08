# $NetBSD: depsrc-optional.mk,v 1.4 2020/11/08 10:17:55 rillig Exp $
#
# Tests for the special source .OPTIONAL in dependency declarations,
# which ignores the target if make cannot find out how to create it.
#
# TODO: Describe practical use cases for this feature.

# TODO: Explain why the commands for "important" are not executed.
# I had thought that only the "optional" commands were skipped.

all: important
	: ${.TARGET} is made.

important: optional optional-cohort
	: ${.TARGET} is made.

optional: .OPTIONAL
	: This is not executed.

# XXX: "non-existent and no sources" is wrong, should be ":: operator and
# no sources..." instead.
optional-cohort:: .OPTIONAL
	: This is not executed.

.MAKEFLAGS: -dm
