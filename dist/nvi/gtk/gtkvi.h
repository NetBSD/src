/*	$NetBSD: gtkvi.h,v 1.1.1.2.2.2 2008/09/17 04:45:24 wrstuden Exp $ */

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
