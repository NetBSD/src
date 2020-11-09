# $NetBSD: sh-leading-at.mk,v 1.4 2020/11/09 20:57:36 rillig Exp $
#
# Tests for shell commands preceded by an '@', to suppress printing
# the command to stdout.

all:
	@
	@echo 'ok'
	@ echo 'space after @'
	echo 'echoed'
	# The leading '@' can be repeated.
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@@@echo '3'
