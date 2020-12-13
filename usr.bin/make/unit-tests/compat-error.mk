# $NetBSD: compat-error.mk,v 1.1 2020/12/13 17:44:31 rillig Exp $
#
# Test detailed error handling in compat mode.
#
# XXX: As of 2020-12-13, .ERROR_TARGET is success3, which is wrong.
#
# XXX: As of 2020-12-13, .ERROR_CMD is empty, which is wrong.
#
# See also:
#	Compat_Run
#
#	The commit that added the NULL command to gn->commands:
#		CVS: 1994.06.06.22.45.??
#		Git: 26a8972fd7f982502c5fbfdabd34578b99d77ca5
#		1994: Lst_Replace (cmdNode, (ClientData) NULL);
#		2020: LstNode_SetNull(cmdNode);
#
#	The commit that skipped NULL commands for .ERROR_CMD:
#		CVS: 2016.08.11.19.53.??
#		Git: 58b23478b7353d46457089e726b07a49197388e4

.MAKEFLAGS: success1 fail1 success2 fail2 success3

success1 success2 success3:
	: Making ${.TARGET} out of nothing.

fail1 fail2:
	: Making ${.TARGET} out of nothing.
	false

.ERROR:
	@echo target: ${.ERROR_TARGET}
	@echo command: ${.ERROR_CMD}
