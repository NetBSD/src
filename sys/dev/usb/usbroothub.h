/* $NetBSD: usbroothub.h,v 1.1.2.1 2014/12/03 23:05:06 skrll Exp $ */

int usb_makestrdesc(usb_string_descriptor_t *, int, const char *);
int usb_makelangtbl(usb_string_descriptor_t *, int);
