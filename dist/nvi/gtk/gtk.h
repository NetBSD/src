/*	$NetBSD: gtk.h,v 1.1.1.2.2.2 2008/09/17 04:45:23 wrstuden Exp $ */

typedef struct {
    GtkViScreen  *vi;
    GtkWidget	*main;
    gint    input_func;
    gint    value_changed;
    IPVI    *ipvi;
    int	    resized;
} gtk_vi;
