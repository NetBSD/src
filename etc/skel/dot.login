#	$NetBSD: dot.login,v 1.1 2000/10/16 13:12:26 simonb Exp $
#csh .login file

if ( ! $?SHELL ) then
  setenv SHELL /bin/csh
endif

set noglob
eval `tset -s -m 'network:?xterm'`
unset noglob
stty status '^T' crt -tostop

/usr/games/fortune
