#!/bin/sh

# $NetBSD: installkernel.sh,v 1.1.4.2 2000/06/22 16:59:52 minoura Exp $

EXT2_MOUNT=/stand
NETBSD_KERNEL=netbsd
FW_KERNELS="vmlinux.gz vmlinux_RAQ.gz vmlinux_raq-2800.gz"

mount | grep -q " on ${EXT2_MOUNT} type ext2fs "

if [ $? -ne 0 ]; then
	echo "WARNING: ${EXT2_MOUNT} is not an ext2 file system"
fi

if [ ! -d ${EXT2_MOUNT}/boot ]; then
	echo "ERROR: ${EXT2_MOUNT}/boot does not exist"
	exit 1
fi

gzip -2 -c /${NETBSD_KERNEL} > ${EXT2_MOUNT}/boot/netbsd.gz

for KERNEL in ${FW_KERNELS}; do
	rm -f ${EXT2_MOUNT}/boot/${KERNEL}
	ln ${EXT2_MOUNT}/boot/${NETBSD_KERNEL}.gz ${EXT2_MOUNT}/boot/${KERNEL}
done
