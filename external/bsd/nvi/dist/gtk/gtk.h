/*	$NetBSD: gtk.h,v 1.2.8.2 2014/08/19 23:51:52 tls Exp $	*/
typedef struct {
    GtkViScreen  *vi;
    GtkWidget	*main;
    gint    input_func;
    gint    value_changed;
    IPVI    *ipvi;
    int	    resized;
} gtk_vi;
