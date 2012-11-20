#!/bin/sh

# Generates various defines needed for using rump on non-NetBSD systems.
# Run this occasionally (yes, it's a slightly suboptimal kludge, but
# better than nothing).

echo Generating rumpdefs.h
rm -f rumpdefs.h
exec > rumpdefs.h

printf '/*	$NetBSD: makerumpdefs.sh,v 1.7.2.1 2012/11/20 03:02:49 tls Exp $	*/\n\n'
printf '/*\n *\tAUTOMATICALLY GENERATED.  DO NOT EDIT.\n */\n\n'
printf '#ifndef _RUMP_RUMPDEFS_H_\n'
printf '#define _RUMP_RUMPDEFS_H_\n\n'
printf '#include <rump/rump_namei.h>\n'
printf '#ifdef _KERNEL\n'
printf '#include <sys/stdint.h>\n'
printf '#else\n'
printf '#include <stdint.h>\n'
printf '#endif\n'

fromvers () {
	echo
	sed -n '1{s/\$//gp;q;}' $1
}

# Odds of sockaddr_in changing are zero, so no acrobatics needed.  Alas,
# dealing with in_addr_t for s_addr is very difficult, so have it as
# an incompatible uint32_t for now.
echo
cat <<EOF
struct rump_sockaddr_in {
	uint8_t		sin_len;
	uint8_t		sin_family;
	uint16_t	sin_port;
	struct {
			uint32_t s_addr;
	} sin_addr;
	int8_t		sin_zero[8];
};
EOF

fromvers ../../../sys/fcntl.h
sed -n '/#define	O_[A-Z]*	*0x/s/O_/RUMP_O_/gp' \
    < ../../../sys/fcntl.h

fromvers ../../../sys/vnode.h
sed -n '/enum vtype.*{/{s/vtype/rump_&/;s/ V/ RUMP_V/gp}' < ../../../sys/vnode.h
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

printf '\n#endif /* _RUMP_RUMPDEFS_H_ */\n'
