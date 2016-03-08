#	$NetBSD: dot.login,v 1.10 2016/03/08 09:51:15 mlelstv Exp $

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend that you create a non-root account and use su(1) for root access."
endif
