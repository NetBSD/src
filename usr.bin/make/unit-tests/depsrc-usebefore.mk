# $NetBSD: depsrc-usebefore.mk,v 1.4 2020/08/22 07:49:44 rillig Exp $
#
# Tests for the special source .USEBEFORE in dependency declarations,
# which allows to prepend common commands to other targets.

all: action directly

first: .USEBEFORE
	@echo first		# Using ${.TARGET} here would expand to "action"

second: .USEBEFORE
	@echo second

# It's possible but uncommon to have a .USEBEFORE target with no commands.
# This may happen as the result of expanding a .for loop.
empty: .USEBEFORE

# It's possible but uncommon to directly make a .USEBEFORE target.
directly: .USEBEFORE
	@echo directly

action: second first empty
