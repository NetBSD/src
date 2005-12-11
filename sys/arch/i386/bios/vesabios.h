/* $NetBSD: vesabios.h,v 1.5 2005/12/11 12:17:40 christos Exp $ */

int vbeprobe(void);

struct vesabiosdev_attach_args {
	const char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
