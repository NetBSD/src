
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>

#undef TRUE
#undef FALSE

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "tsplog.h"


/* Callbacks for the simple password dialog */

void
on_inputdialog1_destroy(GtkObject *object, struct userdata *user_data)
{
	gtk_widget_destroy(user_data->window);
	gtk_main_quit();
}


void
on_dialog1_close(GtkDialog *dialog, struct userdata *user_data)
{
	gtk_widget_destroy(user_data->window);
	gtk_main_quit();
}


void
on_cancelbutton1_clicked(GtkButton *button, struct userdata *user_data)
{
	LogDebugFn();
	gtk_widget_destroy(user_data->window);
	user_data->string_len = 0;
	gtk_main_quit();
}


void
on_okbutton1_clicked(GtkButton *button, struct userdata	*user_data)
{
	const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY(user_data->entry));

	LogDebugFn();
	user_data->string = (char *)Trspi_Native_To_UNICODE((BYTE *)entry_text,
							    &user_data->string_len);
	gtk_widget_destroy(user_data->window);

	gtk_main_quit();
}


gboolean
enter_event(GtkWidget *widget, struct userdata *user_data)
{
	const gchar *entry_text = gtk_entry_get_text (GTK_ENTRY(user_data->entry));

	LogDebugFn();
	user_data->string = (char *)Trspi_Native_To_UNICODE((BYTE *)entry_text,
							    &user_data->string_len);
	gtk_widget_destroy(user_data->window);

	gtk_main_quit();
	return TRUE;
}


/* Callbacks for the new password dialog */
void
on_entryPassword_activate(GtkEntry *entry, struct userdata *user_data)
{
	const gchar *entryPass_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryPass));
	const gchar *entryConf_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryConf));
	int len = strlen(entryConf_text);

	if (strlen(entryConf_text) == strlen(entryPass_text)) {
		if (!memcmp(entryPass_text, entryConf_text, len)) {
			user_data->string = (char *)Trspi_Native_To_UNICODE((BYTE *)entryConf_text,
									    &user_data->string_len);
			gtk_widget_destroy(user_data->window);
			gtk_main_quit();

			LogDebugFn("string len ptr: %p, value = %u", &user_data->string_len,
				   user_data->string_len);
			return;
		}
	}

	gtk_widget_grab_focus(user_data->entryConf);
}

void
on_entryConfirm_activate(GtkEntry *entry, struct userdata *user_data)
{
	const gchar *entryPass_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryPass));
	const gchar *entryConf_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryConf));
	unsigned len = strlen(entryConf_text);

	if (strlen(entryConf_text) == strlen(entryPass_text)) {
		if (!memcmp(entryPass_text, entryConf_text, len)) {
			user_data->string = (char *)Trspi_Native_To_UNICODE((BYTE *)entryConf_text,
									    &user_data->string_len);
			gtk_widget_destroy(user_data->window);
			gtk_main_quit();

			LogDebugFn("string len ptr: %p, value = %u", &user_data->string_len,
				   user_data->string_len);
			return;
		}
	}

	gtk_widget_grab_focus(user_data->entryPass);
}

void
on_cancelbutton2_clicked(GtkButton *button, struct userdata *user_data)
{
	LogDebugFn();
	gtk_widget_destroy(user_data->window);
	user_data->string_len = 0;
	gtk_main_quit();
}

void
on_okbutton2_clicked(GtkButton *button, struct userdata *user_data)
{
	const gchar *entryPass_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryPass));
	const gchar *entryConf_text = gtk_entry_get_text (GTK_ENTRY(user_data->entryConf));
	unsigned len = strlen(entryConf_text);

	if (strlen(entryConf_text) == strlen(entryPass_text)) {
		if (!memcmp(entryPass_text, entryConf_text, len)) {
			user_data->string = (char *)Trspi_Native_To_UNICODE((BYTE *)entryConf_text,
									    &user_data->string_len);
			gtk_widget_destroy(user_data->window);
			gtk_main_quit();

			LogDebugFn("string len ptr: %p, value = %u", &user_data->string_len,
				   user_data->string_len);
			return;
		}
	}

	gtk_widget_grab_focus(user_data->entryPass);
}
