struct romdev {
	int fd;
	int devtype;
	char devname[32];
};

#define DT_BLOCK	1
#define DT_NET		2

int net_open __P((struct romdev *));
int net_close __P((struct romdev *));
int net_mountroot __P((void));
int prom_getether __P((struct romdev *, u_char *));
