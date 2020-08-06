# $NetBSD: recursive.mk,v 1.1 2020/08/06 05:36:33 rillig Exp $
#
# In -dL mode, a variable may get expanded before it makes sense.
# This would stop make from doing anything since the "recursive" error
# is fatal and exits immediately.
#
# The purpose of evaluating that variable early was just to detect
# whether there are unclosed variables.  It might be enough to parse the
# variable value without VARE_WANTRES for that purpose.
#
# Seen in pkgsrc/x11/libXfixes, and probably many more package that use
# GNU Automake.

AM_V_lt = $(am__v_lt_$(V))
am__v_lt_ = $(am__v_lt_$(AM_DEFAULT_VERBOSITY))
am__v_lt_0 = --silent
am__v_lt_1 =
libXfixes_la_LINK = ... $(AM_V_lt) ...
.info not reached

# somewhere later ...
AM_DEFAULT_VERBOSITY = 1
