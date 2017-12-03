/*	$NetBSD: wsconsio.h,v 1.2.122.1 2017/12/03 11:36:33 jdolecek Exp $	*/

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
