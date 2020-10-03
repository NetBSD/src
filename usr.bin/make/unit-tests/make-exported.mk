# $NetBSD: make-exported.mk,v 1.2 2020/10/03 09:48:40 rillig Exp $
#
# As of 2020-08-09, the code in Var_Export is shared between the .export
# directive and the .MAKE.EXPORTED variable.  This leads to non-obvious
# behavior for certain variable assignments.

-env=		make-exported-value-env
-literal=	make-exported-value-literal
UT_VAR=		${UNEXPANDED}

# The following behavior is probably not intended.
.MAKE.EXPORTED=		-env		# behaves like .export-env

# If the value of .MAKE.EXPORTED starts with "-literal", make behaves like
# a mixture of .export-literal and a regular .export.
# XXX: This is due to a sloppy implementation, reusing code in places where
# it is not appropriate.
#
# In Parse_DoVar, the code path for MAKE_EXPORTED is taken, calling Var_Export
# in turn.  There, the code path for .export-literal is taken, and the
# environment variable UT_VAR is set to ${UNEXPANDED}, as expected.
# Later, in Compat_RunCommand, in the child process after vfork,
# Var_ExportVars is called, which treats "-literal" as an ordinary variable
# name, therefore exports it and also overwrites the previously exported
# UT_VAR with the expanded value.
.MAKE.EXPORTED=		-literal UT_VAR

all:
	@env | sort | grep -E '^UT_|make-exported-value' || true
