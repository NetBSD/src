/*	$NetBSD: conf.h,v 1.6 2002/02/27 01:19:06 christos Exp $	*/

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
