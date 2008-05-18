typedef struct {
    GtkViScreen  *vi;
    GtkWidget	*main;
    gint    input_func;
    gint    value_changed;
    IPVI    *ipvi;
    int	    resized;
} gtk_vi;
