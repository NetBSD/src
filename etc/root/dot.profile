#	$NetBSD: dot.profile,v 1.16 2002/10/10 11:15:41 abs Exp $

export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
export PATH=${PATH}:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin

export BLOCKSIZE=1k

if [ -x /usr/bin/tset ]; then
	eval `tset -sQrm 'unknown:?unknown'`
fi


umask 022
#ulimit -c 0

export ENV=/root/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ]; then
        echo "We recommend creating a non-root account and using su(1) for root access."
fi
