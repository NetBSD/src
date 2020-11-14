# $NetBSD: opt-touch.mk,v 1.3 2020/11/14 13:59:58 rillig Exp $
#
# Tests for the -t command line option.

.MAKEFLAGS: -t opt-touch-phony opt-touch-join opt-touch-use

opt-touch-phony: .PHONY
	: Making $@.

opt-touch-join: .JOIN
	: Making $@.

opt-touch-use: .USE
	: Making use of $@.

.END:
	@files=$$(ls opt-touch-* 2>/dev/null | grep -v -e '\.' -e '\*'); \
	[ -z "$$files" ] || { echo "created files: $$files" 1>&2; exit 1; }
