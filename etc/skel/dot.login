#	$NetBSD: dot.login,v 1.1.2.2 2000/10/20 17:00:53 tv Exp $
#csh .login file

if ( ! $?SHELL ) then
  setenv SHELL /bin/csh
endif

set noglob
eval `tset -s -m 'network:?xterm'`
unset noglob
stty status '^T' crt -tostop

/usr/games/fortune
