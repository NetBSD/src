# $NetBSD: varmod-to-upper.mk,v 1.3 2020/08/23 15:18:43 rillig Exp $
#
# Tests for the :tu variable modifier, which returns the words in the
# variable value, converted to uppercase.

# The :tu and :tl modifiers operate on the variable value as a single string,
# not as a list of words. Therefore, the adjacent spaces are preserved.
mod-tu-space:
	@echo $@: ${a   b:L:tu:Q}
