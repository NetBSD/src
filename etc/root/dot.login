#	$NetBSD: dot.login,v 1.4.2.2 2000/02/22 22:27:02 he Exp $

eval `tset -sQrm 'unknown:?unknown'`

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend creating a non-root account and using su(1) for root access."
endif
