/* $NetBSD: vesabios.h,v 1.1 2002/07/07 12:59:58 drochner Exp $ */

int vbeprobe __P((void));

struct vesabios_attach_args {
	char *vaa_busname;
};
