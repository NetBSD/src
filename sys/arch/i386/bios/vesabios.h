/* $NetBSD: vesabios.h,v 1.2.14.2 2005/11/10 13:56:32 skrll Exp $ */

int vbeprobe(void);

struct vesabiosdev_attach_args {
	const char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
