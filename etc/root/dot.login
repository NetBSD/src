#	$NetBSD: dot.login,v 1.4.2.1 2000/02/18 19:32:00 he Exp $

if ( $TERM == unknown ) then
	tset -Q \?$TERM
else
	echo "Terminal type is '$TERM'."
endif

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend creating a non-root account and using su(1) for root access."
endif
