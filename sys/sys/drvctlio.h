/* $NetBSD: drvctlio.h,v 1.2 2005/12/03 17:10:46 christos Exp $ */

/* This interface is experimental and may change. */

#ifndef _SYS_DRVCTLIO_H_ 
#define _SYS_DRVCTLIO_H_ 

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

#endif /* _SYS_DRVCTLIO_H_ */
