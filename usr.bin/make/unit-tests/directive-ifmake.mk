# $NetBSD: directive-ifmake.mk,v 1.3 2020/08/29 19:07:32 rillig Exp $
#
# Tests for the .ifmake directive, which provides a shortcut for asking
# whether a certain target is requested to be made from the command line.

# This is the most basic form.
.ifmake first
.info ok: positive condition works
.else
.warning positive condition fails
.endif

# The not operator works as expected.
# An alternative interpretation were that this condition is asking whether
# the target "!first" was requested.  To distinguish this, see the next test.
.ifmake !first
.warning unexpected
.else
.info ok: negation works
.endif

# See if the exclamation mark really means "not", or if it is just part of
# the target name.
.ifmake !!first
.info ok: double negation works
.else
.warning double negation fails
.endif

# Multiple targets can be combined using the && and || operators.
.ifmake first && second
.info ok: both mentioned
.else
.warning && does not work as expected
.endif

# Negation also works in complex conditions.
.ifmake first && !unmentioned
.info ok: only those mentioned
.else
.warning && with ! does not work as expected
.endif

first second unmentioned:
	: $@
