/* $NetBSD: vesabios.h,v 1.2 2002/07/10 19:15:43 drochner Exp $ */

int vbeprobe __P((void));

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
