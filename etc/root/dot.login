#	$NetBSD: dot.login,v 1.7 2000/02/19 18:39:01 mycroft Exp $

tset -Qrm 'unknown:?unknown'

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend creating a non-root account and using su(1) for root access."
endif
