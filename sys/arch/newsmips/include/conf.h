/*	$NetBSD: conf.h,v 1.5 2000/11/13 16:48:46 tsubai Exp $	*/

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);

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
