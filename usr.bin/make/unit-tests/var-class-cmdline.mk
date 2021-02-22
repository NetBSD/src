# $NetBSD: var-class-cmdline.mk,v 1.3 2021/02/22 22:04:28 rillig Exp $
#
# Tests for variables specified on the command line.
#
# Variables that are specified on the command line override those from the
# global scope.
#
# For performance reasons, variable lookup often starts in the global scope
# since that is where most practically used variables are stored.  But even
# in these cases, variables from the command line scope must override the
# global variables.  Therefore, whenever a global variable is tried to be
# set, it is ignored when there is already a variable of the same name in
# the cmdline scope.  In the same vein, when a cmdline variable is set and
# there is already a variable of the same name in the global scope, that
# global variable is deleted first.
#
# Most cmdline variables are set at the very beginning, when parsing the
# command line arguments.  Using the special target '.MAKEFLAGS', it is
# possible to set cmdline variables at any later time.

# A normal global variable, without any cmdline variable nearby.
VAR=	global
.info ${VAR}

# The global variable is "overridden" by simply deleting it and then
# installing the cmdline variable instead.  Since there is no way to
# undefine a cmdline variable, there is no need to remember the old value
# of the global variable could become visible again.
.MAKEFLAGS: VAR=makeflags
.info ${VAR}

# If Var_SetWithFlags should ever forget to delete the global variable,
# the below line would print "global" instead of the current "makeflags".
.MAKEFLAGS: -V VAR
