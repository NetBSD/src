/*	$NetBSD: gtk.h,v 1.2.4.2 2014/05/22 15:50:35 yamt Exp $	*/
typedef struct {
    GtkViScreen  *vi;
    GtkWidget	*main;
    gint    input_func;
    gint    value_changed;
    IPVI    *ipvi;
    int	    resized;
} gtk_vi;
