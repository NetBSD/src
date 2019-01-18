
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <gtk/gtk.h>

#include "interface.h"

/* Callbacks for the simple text imput dialog */

void
on_dialog1_close                       (GtkDialog       *dialog,
					struct userdata	*user_data);

void
on_cancelbutton1_clicked               (GtkButton       *button,
					struct userdata	*user_data);

void
on_okbutton1_clicked                   (GtkButton       *button,
					struct userdata	*user_data);

gboolean
enter_event		              (GtkWidget       *widget,
					struct userdata	*user_data);

void
on_inputdialog1_destroy                (GtkObject       *object,
					struct userdata	*user_data);

/* Callbacks for the new password dialog */

void
on_entryPassword_activate              (GtkEntry        *entry,
					struct userdata	*user_data);

void
on_entryConfirm_activate               (GtkEntry        *entry,
					struct userdata	*user_data);

void
on_cancelbutton2_clicked               (GtkButton       *button,
					struct userdata	*user_data);

void
on_okbutton2_clicked                   (GtkButton       *button,
					struct userdata	*user_data);

#endif
