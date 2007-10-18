/* $NetBSD: ofw_cons.h,v 1.1.8.1 2007/10/18 08:32:38 yamt Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

void ofwoea_consinit(void);
int ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
