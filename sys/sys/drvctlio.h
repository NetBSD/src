/* $NetBSD: drvctlio.h,v 1.1 2004/08/18 12:19:30 drochner Exp $ */

/* This interface is experimental and may change. */

#define DRVCTLDEV "/dev/drvctl"

struct devdetachargs {
	char devname[16];
};

struct devrescanargs {
	char busname[16];
	char ifattr[16];
	unsigned int numlocators;
	int *locators;
};

#define DRVDETACHDEV _IOW('D', 123, struct devdetachargs)
#define DRVRESCANBUS _IOW('D', 124, struct devrescanargs)
