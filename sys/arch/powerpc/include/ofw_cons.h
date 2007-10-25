/* $NetBSD: ofw_cons.h,v 1.1.10.1 2007/10/25 22:36:26 bouyer Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

void ofwoea_consinit(void);
int ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
