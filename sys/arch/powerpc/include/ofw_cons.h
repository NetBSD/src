/* $NetBSD: ofw_cons.h,v 1.2.4.2 2007/10/27 11:27:47 yamt Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

void ofwoea_consinit(void);
int ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
