/*
 * Simple Transfer Manager
 * -----------------------
 *
 * Copyright (C) 2008 Przemysław Sitek
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include "stm.h"
#include "stm-new-transfer-window.h"


G_DEFINE_TYPE (StmNewTransferWindow, stm_new_transfer_window, GTK_TYPE_DIALOG)

struct _StmNewTransferWindowPrivate
{
	/* Private members go here */
	
	GtkEntry				*source;
	GtkFileChooserButton	*destination;
	GtkCheckButton			*auto_start;

	gboolean disposed;
};


#define STM_NEW_TRANSFER_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_NEW_TRANSFER_WINDOW, StmNewTransferWindowPrivate))


GtkWidget*
stm_new_transfer_window_new (void)
{
	StmNewTransferWindow *self = g_object_new (STM_TYPE_NEW_TRANSFER_WINDOW, NULL);
	return GTK_WIDGET (self);
}


const gchar *
stm_new_transfer_window_get_uri (StmNewTransferWindow *self)
{
	StmNewTransferWindowPrivate *priv = self->priv;
	return gtk_entry_get_text (priv->source);
}


const gchar *
stm_new_transfer_window_get_destination (StmNewTransferWindow *self)
{
	StmNewTransferWindowPrivate *priv = self->priv;
	return gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->destination));
}


gboolean
stm_new_transfer_window_get_auto_start (StmNewTransferWindow *self)
{
	StmNewTransferWindowPrivate *priv = self->priv;
	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->auto_start));
}

static void
stm_new_transfer_window_close_clicked (GtkButton *button, StmNewTransferWindow *self)
{
	g_print ("Self-destruction\n");
	gtk_widget_destroy (GTK_WIDGET (self));
	g_print ("Self-destruction: DONE\n");
}


static void
stm_new_transfer_window_ui (StmNewTransferWindow *self)
{
	StmNewTransferWindowPrivate *priv = self->priv;
	
	gtk_window_set_title (GTK_WINDOW (self), _("Add transfer"));

	/* Buttons */
	GtkWidget *close = gtk_dialog_add_button (GTK_DIALOG (self),
	                                          GTK_STOCK_CANCEL,
	                                          GTK_RESPONSE_CANCEL);
	g_signal_connect (G_OBJECT (close), "clicked",
	                  G_CALLBACK (stm_new_transfer_window_close_clicked), self);

	/* Table */
	GtkWidget *table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table), 12);
	GtkWidget *label;
	
	/* Left column */
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>URL:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 0, 1,
	                  GTK_FILL, GTK_SHRINK, 0, 0);

	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Save to:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 1, 2,
	                  GTK_FILL, GTK_SHRINK, 0, 0);

	/* Right column */
	GtkWidget *entry;
	entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table), entry,
	                  1, 2, 0, 1,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->source = GTK_ENTRY (entry);
	
	entry = gtk_file_chooser_button_new (_("Select destination"),
	                                     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_table_attach (GTK_TABLE (table), entry,
	                  1, 2, 1, 2,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->destination = GTK_FILE_CHOOSER_BUTTON (entry);
	
	entry = gtk_check_button_new_with_label (_("Automatically start transfer"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), TRUE);
	gtk_table_attach (GTK_TABLE (table), entry,
	                  0, 2, 2, 3,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->auto_start = GTK_CHECK_BUTTON (entry);
	
	gtk_widget_show_all (table);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), table,
	                    FALSE, FALSE, 0);
	/* Buttons */
	GtkWidget *ok = gtk_dialog_add_button (GTK_DIALOG (self),
	                                       GTK_STOCK_OK,
	                                       GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (ok), "clicked",
	                  G_CALLBACK (stm_new_transfer_window_close_clicked), self);
}

/* GObject inplementation */


static void
stm_new_transfer_window_init (StmNewTransferWindow *self)
{
	self->priv = STM_NEW_TRANSFER_WINDOW_GET_PRIVATE (self);
	StmNewTransferWindowPrivate *priv = self->priv;

	priv->disposed = FALSE;
	
	stm_new_transfer_window_ui (self);
}


static void
stm_new_transfer_window_dispose (GObject *object)
{
	StmNewTransferWindow *self = (StmNewTransferWindow*) object;
	StmNewTransferWindowPrivate *priv = self->priv;


	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;


	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_new_transfer_window_parent_class)->dispose (object);
}


static void
stm_new_transfer_window_finalize (GObject *object)
{
	G_OBJECT_CLASS (stm_new_transfer_window_parent_class)->finalize (object);
}

	
static void
stm_new_transfer_window_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
//	StmNewTransferWindow* self = STM_NEW_TRANSFER_WINDOW (object);
//	StmNewTransferWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_new_transfer_window_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
//	StmNewTransferWindow* self = STM_NEW_TRANSFER_WINDOW (object);
//	StmNewTransferWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_new_transfer_window_class_init (StmNewTransferWindowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_new_transfer_window_get_property;
	gobject_class->set_property = stm_new_transfer_window_set_property;
	gobject_class->dispose = stm_new_transfer_window_dispose;
	gobject_class->finalize = stm_new_transfer_window_finalize;

	g_type_class_add_private (klass, sizeof (StmNewTransferWindowPrivate));
}

