/*	$NetBSD: gtk.h,v 1.1.1.1.2.3 2008/06/04 02:03:16 yamt Exp $ */

typedef struct {
    GtkViScreen  *vi;
    GtkWidget	*main;
    gint    input_func;
    gint    value_changed;
    IPVI    *ipvi;
    int	    resized;
} gtk_vi;
