# $NetBSD: dep.mk,v 1.5 2026/06/09 07:30:38 rillig Exp $
#
# Tests for dependency declarations, such as "target: sources".

.MAIN: all

# As soon as a target is defined using one of the dependency operators, it is
# restricted to this dependency operator and cannot use the others anymore.
only-colon:
# expect+1: Inconsistent operator for only-colon
only-colon!
# expect+1: Inconsistent operator for only-colon
only-colon::
# Ensure that the target still has the original operator.  If it hadn't, there
# would be another error message.
only-colon:


# Before parse.c 1.158 from 2009-10-07, the parser broke dependency lines at
# the first ';', without parsing expressions as such.  It interpreted the
# first ';' as the separator between the dependency and its commands, and the
# '^' as a shell command.
all: for-subst
.for file in ${.PARSEFILE}
for-subst:	  ${file:S;^;./;g}
	@echo ".for with :S;... OK"
.endfor


# Before parse.c 1.757 from 2026-06-09, the parser counted "$(" and "${" as
# opening a brace level, and ")" and "}" as closing it.  It didn't count a
# plain "(" or "{" as opening, though.  This simple counting resulted in
# parse errors on modifiers containing parentheses or braces, as well as
# semicolons.

semicolon-in-modifier.${:Utarget:S;from;to;}:
# FIXME
# expect+3: Unfinished modifier after "(.)", expecting ";"
# FIXME
# expect+1: Invalid line "parentheses-in-modifier.${:Utarget:C;(.)", expanded to "parentheses-in-modifier."
parentheses-in-modifier.${:Utarget:C;(.);\1-\1;g}:
# FIXME
# expect+1: Invalid line "var-semicolon.$", expanded to "var-semicolon."
var-semicolon.$;: source
# FIXME
# expect+1: Invalid line "double-dollar.$$$", expanded to "double-dollar.$"
double-dollar.$$$;-$$$$: source

all:
