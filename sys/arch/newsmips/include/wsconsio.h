/*	$NetBSD: wsconsio.h,v 1.2.142.1 2015/09/22 12:05:48 skrll Exp $	*/

#ifndef _NEWSMIPS_WSCONSIO_H_
#define	_NEWSMIPS_WSCONSIO_H_

#include <sys/ioccom.h>
#include <dev/wscons/wsconsio.h>

struct newsmips_wsdisplay_fbinfo {
	struct wsdisplay_fbinfo wsdisplay_fbinfo;
	u_int stride;
};

#define	NEWSMIPS_WSDISPLAYIO_GINFO						\
	_IOR('n', 65, struct newsmips_wsdisplay_fbinfo)

#endif /* !_NEWSMIPS_WSCONSIO_H_ */
