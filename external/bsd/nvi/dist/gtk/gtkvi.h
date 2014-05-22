/*	$NetBSD: gtkvi.h,v 1.2.4.2 2014/05/22 15:50:35 yamt Exp $	*/
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
