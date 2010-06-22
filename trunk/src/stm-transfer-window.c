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
#include "stm-transfer-window.h"


G_DEFINE_TYPE (StmTransferWindow, stm_transfer_window, GTK_TYPE_DIALOG)

struct _StmTransferWindowPrivate
{
	/* Private members go here */
	StmTransfer 		*transfer;
	
	GtkWidget			*source;
	GtkWidget			*destination;
	GtkWidget			*status;
	GtkWidget			*time1;
	GtkWidget			*time2;
	GtkWidget			*progress;
#ifdef HAVE_CRYPTO
	GtkWidget			*md5;	
#endif
	
	gboolean			 disposed;
};


#define STM_TRANSFER_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_TRANSFER_WINDOW, StmTransferWindowPrivate))


GtkWidget*
stm_transfer_window_new (void)
{
	StmTransferWindow *self = g_object_new (STM_TYPE_TRANSFER_WINDOW, NULL);
	return GTK_WIDGET (self);
}


static void
stm_transfer_window_close_clicked (GtkButton *button, StmTransferWindow *self)
{
	g_print ("Self-destruction\n");
	gtk_widget_destroy (GTK_WIDGET (self));
	g_print ("Self-destruction: DONE\n");
}


static void
stm_transfer_window_progress (StmTransfer *transfer, StmTransferWindow *self)
{
	StmTransferWindowPrivate *priv = self->priv;
	
	gchar buf1[10];
	gchar buf2[10];
	gchar buf3[10];
	gchar buf4[128];

	const gchar *file_name	= stm_transfer_get_file_name (transfer);
	guint64 downloaded		= (guint64) stm_transfer_get_downloaded (transfer);
	guint64 content_length	= (guint64) stm_transfer_get_content_length (transfer);
	guint64 speed			= (guint64) stm_transfer_get_speed (transfer);
	guint64 total			= (guint64) stm_transfer_get_total_time (transfer);
	guint64 eta				= (guint64) stm_transfer_get_eta (transfer);

	double r		= (content_length != 0) ? ((double)downloaded / content_length) : 0.0;
	
	gchar *percent = g_strdup_printf ("%d%%", (int) (100*r));
	gchar *status = g_strdup_printf ("Downloaded %s bytes of %s at %s/s",
	                                 stm_format_size_buffer (downloaded, buf1, 10),
	                                 stm_format_size_buffer (content_length, buf2, 10),
	                                 stm_format_size_buffer (speed, buf3, 10));
	gchar *title = g_strdup_printf ("%s (%s)", file_name, percent);
	
	gtk_window_set_title (GTK_WINDOW (self), title);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress), r);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress), percent);
	gtk_label_set_text (GTK_LABEL (priv->status), status);
	gtk_label_set_text (GTK_LABEL (priv->time1),
	                    stm_format_time_buffer (total, buf4, 128));
	gtk_label_set_text (GTK_LABEL (priv->time2),
	                    stm_format_time_buffer (eta, buf4, 128));
	
#ifdef HAVE_CRYPTO
	const gchar *md5 = stm_transfer_get_md5 (transfer);
	if (md5)
		gtk_label_set_text (GTK_LABEL (priv->md5), md5);
	else
		gtk_label_set_markup (GTK_LABEL (priv->md5), _("<i>unavailable</i>"));
#endif
	
	g_free (percent);
	g_free (status);
	g_free (title);
}


/**
 * stm_transfer_window_set_transfer:
 * 
 * @self A #StmTransferWindow
 * @transfer A #StmTransfer
 * 
 * Set transfer that is displayed in transfer window
 */
void
stm_transfer_window_set_transfer (StmTransferWindow *self, StmTransfer *transfer)
{
	g_return_if_fail (self);
//	g_return_if_fail (STM_IS_TRANSFER_WINDOW (self));
	
	StmTransferWindowPrivate *priv = self->priv;
	
	if (priv->transfer != NULL) {
		g_signal_handlers_disconnect_by_func (G_OBJECT (priv->transfer),
		                                      G_CALLBACK (stm_transfer_window_progress),
		                                      self);
		g_object_unref (priv->transfer);
	}
	
	if (transfer != NULL) {
		priv->transfer = g_object_ref (transfer);
	
		g_signal_connect (G_OBJECT (transfer), "progress",
	    	              G_CALLBACK (stm_transfer_window_progress), self);

		/* Update UI */
		gtk_label_set_text (GTK_LABEL (priv->source), stm_transfer_get_uri (transfer));
		gtk_label_set_text (GTK_LABEL (priv->destination), stm_transfer_get_file (transfer));
		stm_transfer_window_progress (transfer, self);
	} else {
		priv->transfer = NULL;
	}
}


/**
 * stm_transfer_window_init_ui:
 * 
 * Initialize user interface
 */
static void
stm_transfer_window_init_ui (StmTransferWindow *self)
{
	StmTransferWindowPrivate *priv = self->priv;
	
	GtkWidget *table = gtk_table_new (6, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table), 12);
	GtkWidget *label;
	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Source:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 0, 1,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	
	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Destination:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 1, 2,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Status:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 2, 3,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Total time:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 3, 4,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Time remaining:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 4, 5,
	                  GTK_FILL, GTK_SHRINK, 0, 0);

#ifdef HAVE_CRYPTO	
	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>MD5 Checksum:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 5, 6,
	                  GTK_FILL, GTK_SHRINK, 0, 0);
#endif

	label = gtk_label_new ("");
	gtk_label_set_markup (GTK_LABEL (label), _("<b>Progress:</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
	                  0, 1, 6, 7,
	                  GTK_FILL, GTK_SHRINK, 0, 0);

	/* Right column */

	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 0, 1,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->source = label;

	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 1, 2,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->destination = label;

	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 2, 3,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->status = label;

	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 3, 4,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->time1 = label;

	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 4, 5,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->time2 = label;

#ifdef HAVE_CRYPTO
	label = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_table_attach (GTK_TABLE (table), label,
	                  1, 2, 5, 6,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->md5 = label;
#endif

	GtkWidget *progress;
	progress = gtk_progress_bar_new ();
	gtk_table_attach (GTK_TABLE (table), progress,
	                  1, 2, 6, 7,
	                  GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
	priv->progress = progress;
	
	
	gtk_widget_show_all (table);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), table,
	                    FALSE, FALSE, 0);

	/* Buttons */
	GtkWidget *close = gtk_dialog_add_button (GTK_DIALOG (self),
	                                          GTK_STOCK_CLOSE,
	                                          GTK_RESPONSE_CLOSE);
	g_signal_connect (G_OBJECT (close), "clicked",
	                  G_CALLBACK (stm_transfer_window_close_clicked), self);
}


static void
stm_transfer_window_init (StmTransferWindow *self)
{
	self->priv = STM_TRANSFER_WINDOW_GET_PRIVATE (self);
	StmTransferWindowPrivate *priv = self->priv;
	
	priv->transfer = NULL;
	stm_transfer_window_init_ui (self);

	priv->disposed = FALSE;
}


static void
stm_transfer_window_dispose (GObject *object)
{
	StmTransferWindow *self = (StmTransferWindow*) object;
	StmTransferWindowPrivate *priv = self->priv;

	g_print ("stm_transfer_window_dispose\n");

	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;
	
	stm_transfer_window_set_transfer (self, NULL);


	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_transfer_window_parent_class)->dispose (object);
}


static void
stm_transfer_window_finalize (GObject *object)
{
	g_print ("stm_transfer_window_finalize\n");
	G_OBJECT_CLASS (stm_transfer_window_parent_class)->finalize (object);
}

	
static void
stm_transfer_window_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
//	StmTransferWindow* self = STM_TRANSFER_WINDOW (object);
//	StmTransferWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_transfer_window_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
//	StmTransferWindow* self = STM_TRANSFER_WINDOW (object);
//	StmTransferWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_transfer_window_class_init (StmTransferWindowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_transfer_window_get_property;
	gobject_class->set_property = stm_transfer_window_set_property;
	gobject_class->dispose = stm_transfer_window_dispose;
	gobject_class->finalize = stm_transfer_window_finalize;

	g_type_class_add_private (klass, sizeof (StmTransferWindowPrivate));
}

