/*	$NetBSD: conf.h,v 1.5.8.1 2002/02/28 04:11:16 nathanw Exp $	*/

#include <sys/conf.h>

bdev_decl(sw);
cdev_decl(sw);

bdev_decl(raid);
cdev_decl(raid);

bdev_decl(fd);
cdev_decl(fd);

cdev_decl(zs);

cdev_decl(scsibus);

cdev_decl(wsdisplay);
cdev_decl(wskbd);
cdev_decl(wsmouse);
cdev_decl(wsmux);
