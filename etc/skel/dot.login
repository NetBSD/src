#	$NetBSD: dot.login,v 1.1.4.2 2000/10/25 16:58:50 he Exp $
#csh .login file

if ( ! $?SHELL ) then
  setenv SHELL /bin/csh
endif

set noglob
eval `tset -s -m 'network:?xterm'`
unset noglob
stty status '^T' crt -tostop

/usr/games/fortune
