#	$NetBSD: dot.profile,v 1.14 2000/02/19 18:39:01 mycroft Exp $

export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
export PATH=${PATH}:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin

export BLOCKSIZE=1k

if [ -x /usr/bin/tset ]; then
	tset -Qrm 'unknown:?unknown'
fi


umask 022
#ulimit -c 0

export ENV=$HOME/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
        echo "We recommend creating a non-root account and using su(1) for root access."
fi
