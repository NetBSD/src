#!/bin/sh

# Generates various defines needed for using rump on non-NetBSD systems.
# Run this occasionally (yes, it's a slightly suboptimal kludge, but
# better than nothing).

echo Generating rumpdefs.h
rm -f rumpdefs.h
exec > rumpdefs.h

printf '/*	$NetBSD: makerumpdefs.sh,v 1.21 2013/08/15 22:10:04 pooka Exp $	*/\n\n'
printf '/*\n *\tAUTOMATICALLY GENERATED.  DO NOT EDIT.\n */\n\n'
printf '#ifndef _RUMP_RUMPDEFS_H_\n'
printf '#define _RUMP_RUMPDEFS_H_\n\n'
printf '#include <rump/rump_namei.h>\n'

fromvers () {
	echo
	sed -n '1{s/\$//gp;q;}' $1
}

# not perfect, but works well enough for the cases so far
# (also has one struct-specific hack for MAXNAMLEN)
getstruct () {
	sed -n '/struct[ 	]*'"$2"'[ 	]*{/{
		a\
struct rump_'"$2"' {
		:loop
		n
		s/^}.*;$/};/p
		t
		/^#/!{/MAXNAMLEN/!{s/ino_t/uint64_t/;p;}}
		b loop
	}' < $1
}

fromvers ../../../sys/fcntl.h
sed -n '/#define	O_[A-Z]*	*0x/s/O_/RUMP_O_/gp' \
    < ../../../sys/fcntl.h

fromvers ../../../sys/vnode.h
sed -n '/enum vtype.*{/{s/vtype/rump_&/;s/ V/ RUMP_V/gp;}' <../../../sys/vnode.h
sed -n '/#define.*LK_[A-Z]/s/LK_/RUMP_LK_/gp' <../../../sys/vnode.h	\
    | sed 's,/\*.*$,,'

fromvers ../../../sys/errno.h
sed -n '/#define[ 	]*E/s/E[A-Z]*/RUMP_&/p' < ../../../sys/errno.h

fromvers ../../../sys/reboot.h
sed -n '/#define.*RB_[A-Z]/s/RB_/RUMP_RB_/gp' <../../../sys/reboot.h	\
    | sed 's,/\*.*$,,'
sed -n '/#define.*AB_[A-Z]/s/AB_/RUMP_AB_/gp' <../../../sys/reboot.h	\
    | sed 's,/\*.*$,,'

fromvers ../../../sys/socket.h
sed -n '/#define[ 	]*SOCK_[A-Z]/s/SOCK_/RUMP_SOCK_/gp' <../../../sys/socket.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*[AP]F_[A-Z]/s/[AP]F_/RUMP_&/gp' <../../../sys/socket.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*SO_[A-Z]/s/SO_/RUMP_&/gp' <../../../sys/socket.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*SOL_[A-Z]/s/SOL_/RUMP_&/gp' <../../../sys/socket.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*MSG_[A-Z]/s/MSG_/RUMP_&/gp' <../../../sys/socket.h \
    | sed 's,/\*.*$,,'

fromvers ../../../netinet/in.h
sed -n '/#define[ 	]*IP_[A-Z]/s/IP_/RUMP_&/gp' <../../../netinet/in.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*IPPROTO_[A-Z]/s/IPPROTO_/RUMP_&/gp' <../../../netinet/in.h \
    | sed 's,/\*.*$,,'

fromvers ../../../netinet/tcp.h
sed -n '/#define[ 	]*TCP_[A-Z]/s/TCP_/RUMP_&/gp' <../../../netinet/tcp.h \
    | sed 's,/\*.*$,,'

fromvers ../../../sys/mount.h
sed -n '/#define[ 	]*MOUNT_[A-Z]/s/MOUNT_/RUMP_MOUNT_/gp' <../../../sys/mount.h | sed 's,/\*.*$,,'

fromvers ../../../sys/fstypes.h
sed -n '/#define[ 	]*MNT_[A-Z].*[^\]$/s/MNT_/RUMP_MNT_/gp' <../../../sys/fstypes.h | sed 's,/\*.*$,,'

fromvers ../../../sys/ioccom.h
sed -n '/#define[ 	]*IOC[A-Z_]/s/IOC/RUMP_&/gp' <../../../sys/ioccom.h | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*_IO.*\\$/{:t;N;/\\$/bt;s/_IOC/_RUMP_IOC/g;s/IOC[A-Z]/RUMP_&/gp}' <../../../sys/ioccom.h \
    | sed 's,/\*.*$,,'
sed -n '/#define[ 	]*_IO.*[^\]$/{s/_IO/_RUMP_IO/g;s/IOC_/RUMP_IOC_/gp}' <../../../sys/ioccom.h \
    | sed 's,/\*.*$,,'

fromvers ../../../sys/module.h
getstruct ../../../sys/module.h modctl_load

fromvers ../../../ufs/ufs/ufsmount.h
getstruct ../../../ufs/ufs/ufsmount.h ufs_args

fromvers ../../../fs/sysvbfs/sysvbfs_args.h
getstruct ../../../fs/sysvbfs/sysvbfs_args.h sysvbfs_args

fromvers ../../../sys/dirent.h
getstruct ../../../sys/dirent.h dirent

printf '\n#endif /* _RUMP_RUMPDEFS_H_ */\n'
