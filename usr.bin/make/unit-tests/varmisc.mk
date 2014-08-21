# $Id: varmisc.mk,v 1.1 2014/08/21 15:37:13 apb Exp $
#
# Miscellaneous variable tests.

all: unmatched_var_paren

unmatched_var_paren:
	@echo $(foo::=foo-text)
