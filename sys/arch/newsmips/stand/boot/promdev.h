struct romdev {
	int fd;
	int devtype;
	char devname[32];
};

#define DT_BLOCK	1
#define DT_NET		2

int net_open(struct romdev *);
int net_close(struct romdev *);
int net_mountroot(void);
int prom_getether(struct romdev *, u_char *);
