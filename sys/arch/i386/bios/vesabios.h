/* $NetBSD: vesabios.h,v 1.2.14.1 2005/02/04 11:44:19 skrll Exp $ */

int vbeprobe(void);

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
