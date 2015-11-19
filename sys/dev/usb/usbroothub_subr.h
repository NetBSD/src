/* $NetBSD: usbroothub_subr.h,v 1.3 2015/11/19 00:40:43 christos Exp $ */

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);
