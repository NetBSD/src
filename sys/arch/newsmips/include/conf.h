
#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);

bdev_decl(sw);
cdev_decl(sw);

bdev_decl(fd);
cdev_decl(fd);

cdev_decl(zs);
cdev_decl(fb);
cdev_decl(bmcn);
cdev_decl(ms);

cdev_decl(scsibus);
