/* $NetBSD: if_wmvar.h,v 1.1 2006/02/16 00:02:00 gavan Exp $ */

#ifndef _DEV_PCI_IF_WMVAR_H_
#define _DEV_PCI_IF_WMVAR_H_

#ifdef __HAVE_WM_READ_EEPROM_HOOK
extern int  wm_read_eeprom_hook(int, int, u_int16_t *);
#endif /* __HAVE_WM_READ_EEPROM_HOOK */

#endif /* _DEV_PCI_IF_WMVAR_H_ */
