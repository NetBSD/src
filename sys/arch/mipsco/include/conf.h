/*	$NetBSD: conf.h,v 1.1.6.1 2002/03/16 15:58:46 jdolecek Exp $	*/

#include <sys/conf.h>

bdev_decl(sw);
cdev_decl(sw);

bdev_decl(raid);
cdev_decl(raid);

bdev_decl(fd);
cdev_decl(fd);

cdev_decl(zs);
cdev_decl(fb);
cdev_decl(bmcn);
cdev_decl(ms);

cdev_decl(scsibus);
