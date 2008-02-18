/* $NetBSD: usbroothub_subr.h,v 1.1.4.2 2008/02/18 21:06:27 mjf Exp $ */

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);
