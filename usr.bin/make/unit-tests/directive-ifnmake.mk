# $NetBSD: directive-ifnmake.mk,v 1.3 2020/10/04 22:41:18 rillig Exp $
#
# Tests for the .ifnmake directive, which evaluates to true if its argument
# is _not_ listed in the command-line targets to be created.

all:
	@:;

.ifnmake(test)
.BEGIN:
	@echo "Don't forget to run the tests (1)"
.endif

.MAKEFLAGS: test

.ifnmake(test)
.BEGIN:
	@echo "Don't forget to run the tests (2)"
.endif

test:
	@echo "Running the tests"
