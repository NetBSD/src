#	$NetBSD: dot.login,v 1.8 2000/02/19 19:00:13 mycroft Exp $

eval `tset -sQrm 'unknown:?unknown'`

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend creating a non-root account and using su(1) for root access."
endif
