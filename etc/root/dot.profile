#	$NetBSD: dot.profile,v 1.35 2022/12/25 23:58:50 nia Exp $

case "${PATH}" in
/rescue:*)	;; # leave it alone, user can change manually (if required)
*)	export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
	export PATH=${PATH}:/usr/games:/usr/X11R7/bin
	export PATH=${PATH}:/usr/local/sbin:/usr/local/bin
	;;
esac

# Uncomment the following line(s) to install binary packages
# from cdn.NetBSD.org via pkg_add.  (See also pkg_install.conf)
#export PKG_PATH="https://cdn.NetBSD.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r|cut -f '1 2' -d.|cut -f 1 -d_)/All"

export BLOCKSIZE=1k

export HOST="$(hostname)"

umask 022
#ulimit -c 0

export ENV=/root/.shrc

# Do not display in 'su -' case
if [ -z "$SU_FROM" ] && [ "$PPID" -ne 1 ]; then
        echo "We recommend that you create a non-root account and use su(1) for root access."
fi
