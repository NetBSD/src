/*	$NetBSD: extern.h,v 1.1.1.1.2.2 2008/06/04 02:03:16 yamt Exp $ */

/* Do not edit: automatically built by build/distrib. */
int gtk_vi_init __P((GtkVi **, int, char*[]));
void gtk_vi_quit __P((GtkViWindow*, gint));
void gtk_vi_show_term __P((GtkViWindow*, gint));
void gtk_vi_key_press_event __P((GtkViWindow*, GdkEventKey*));
