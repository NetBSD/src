#!/bin/sh
# $Id: install.sh,v 1.2 1994/04/18 12:10:54 deraadt Exp $
umask 0
cat ./bin.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./dev.cpio.gz | gzip -d | (cd /mnt; cpio -iduv)
cat ./etc.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./sbin.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.bin.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.games.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.include.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.lib.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.libexec.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.misc.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.sbin.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./usr.share.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cat ./var.tar.gz | gzip -d | (cd /mnt; tar xvpf -)
cp ./netbsd-sd0 /mnt/netbsd-sd0
cp ./netbsd-sd1 /mnt/netbsd-sd1
(cd /mnt; ln -s netbsd-sd0 netbsd)
