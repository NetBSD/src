/* $NetBSD: ofw_cons.h,v 1.2.126.1 2021/04/03 22:28:34 thorpej Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

void	ofwoea_cnprobe(void);
void	ofwoea_consinit(void);
int	ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
