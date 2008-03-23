/* $NetBSD: usbroothub_subr.h,v 1.1.12.2 2008/03/23 02:04:54 matt Exp $ */

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);
