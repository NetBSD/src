/* $NetBSD: vesabios.h,v 1.2.2.2 2002/07/16 08:29:06 gehenna Exp $ */

int vbeprobe __P((void));

struct vesabios_attach_args {
	char *vaa_busname;
};

struct vesabiosdev_attach_args {
	char *vbaa_type;
	int *vbaa_modes;
	int vbaa_nmodes;
};
