/* $NetBSD: usbroothub_subr.h,v 1.1.2.2 2008/02/04 09:23:41 yamt Exp $ */

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);
