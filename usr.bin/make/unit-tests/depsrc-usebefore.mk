# $NetBSD: depsrc-usebefore.mk,v 1.3 2020/08/22 07:46:18 rillig Exp $
#
# Tests for the special source .USEBEFORE in dependency declarations,
# which allows to prepend common commands to other targets.

all: action directly

first: .USEBEFORE
	@echo first		# Using ${.TARGET} here would expand to "action"

second: .USEBEFORE
	@echo second

empty: .USEBEFORE

# It's possible but uncommon to directly make a .USEBEFORE target.
directly: .USEBEFORE
	@echo directly

action: second first empty
