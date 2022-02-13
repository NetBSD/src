/* $NetBSD: ofw_cons.h,v 1.4 2022/02/13 12:24:24 martin Exp $ */

#ifndef _POWERPC_OFW_CONS_H_
#define _POWERPC_OFW_CONS_H_

extern bool ofwoea_use_serial_console;
void	ofwoea_cnprobe(void);
void	ofwoea_consinit(void);
int	ofkbd_cngetc(dev_t dev);

#endif /* _POWERPC_OFW_CONS_H_ */
