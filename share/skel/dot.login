#	$NetBSD: dot.login,v 1.2 1997/10/17 09:26:59 mrg Exp $
#csh .login file

setenv SHELL /bin/csh
set noglob
eval `tset -s -m 'network:?xterm'`
unset noglob
stty status '^T' crt -tostop

/usr/games/fortune
