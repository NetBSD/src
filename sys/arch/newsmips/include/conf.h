/*	$NetBSD: conf.h,v 1.5.4.1 2002/03/16 15:59:03 jdolecek Exp $	*/

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
