/* $NetBSD: ofw_cons.h,v 1.2 2007/10/17 19:56:41 garbled Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

void ofwoea_consinit(void);
int ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
