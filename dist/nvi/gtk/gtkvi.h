/*	$NetBSD: gtkvi.h,v 1.1.1.1.2.3 2008/06/04 02:03:16 yamt Exp $ */

#ifndef __GTK_VI_H__
#define __GTK_VI_H__

typedef struct _GtkVi           GtkVi;

struct _GtkVi
{
//    GtkWidget	*term;
//    GtkWidget	*vi;	    /* XXX */
//    GtkWidget	*vi_window;
    IPVI    *ipvi;
};
#endif /* __GTK_VI_H__ */
