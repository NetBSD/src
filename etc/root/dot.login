#	$NetBSD: dot.login,v 1.6 2000/02/16 03:07:09 jwise Exp $

if ( $TERM == unknown ) then
	tset -Q \?$TERM
else
	echo "Terminal type is '$TERM'."
	tset -Q $TERM
endif

# Do not display in 'su -' case
if ( ! $?SU_FROM ) then
	echo "We recommend creating a non-root account and using su(1) for root access."
endif
