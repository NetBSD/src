# $NetBSD: depsrc-use.mk,v 1.3 2020/08/22 07:49:44 rillig Exp $
#
# Tests for the special source .USE in dependency declarations,
# which allows to append common commands to other targets.

all: action directly

first: .USE
	@echo first		# Using ${.TARGET} here would expand to "action"

second: .USE
	@echo second

# It's possible but uncommon to have a .USE target with no commands.
# This may happen as the result of expanding a .for loop.
empty: .USE

# It's possible but uncommon to directly make a .USEBEFORE target.
directly: .USE
	@echo directly

action: first second empty
