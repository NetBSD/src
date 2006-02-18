/* $NetBSD: if_wmvar.h,v 1.1.2.2 2006/02/18 15:39:08 yamt Exp $ */

#ifndef _DEV_PCI_IF_WMVAR_H_
#define _DEV_PCI_IF_WMVAR_H_

#ifdef __HAVE_WM_READ_EEPROM_HOOK
extern int  wm_read_eeprom_hook(int, int, u_int16_t *);
#endif /* __HAVE_WM_READ_EEPROM_HOOK */

#endif /* _DEV_PCI_IF_WMVAR_H_ */
