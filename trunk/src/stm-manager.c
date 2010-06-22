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
#include <stdio.h>
#include <string.h>
#include "stm-manager.h"
#include "stm-private-api.h"
#include "glibcurl.h"


G_DEFINE_TYPE (StmManager, stm_manager, G_TYPE_OBJECT)

struct _StmManagerPrivate
{
	/* Private members go here */
	GList		*transfers;		/* list of transfers */
	GtkTreeModel	*model;			/* Tree model singleton */

	gchar		*state_file;		/* State file */
	guint		 state_file_id;		/* ID of idle function writing state file */

	gboolean disposed;
};


#define STM_MANAGER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_MANAGER, StmManagerPrivate))


/* Signals */
enum {
	TRANSFER_ADDED,
	TRANSFER_REMOVED,
	
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

StmManager*
stm_manager_new (void)
{
	StmManager *self = g_object_new (STM_TYPE_MANAGER, NULL);
	return self;
}


/**
 * stm_manager_add_transfer:
 * 
 * @self A #StmManager
 * @transfer A #StmTransfer
 * 
 * Add a transfer to manager.
 */
void
stm_manager_add_transfer (StmManager *self, StmTransfer *transfer)
{
	StmManagerPrivate *priv = self->priv;

	priv->transfers = g_list_append (priv->transfers, 
	                                 g_object_ref (transfer));
//	stm_transfer_start (transfer);

	g_signal_emit (self, signals[TRANSFER_ADDED], 0, transfer);
}


/**
 * stm_manager_remove_transfer:
 * 
 * @self: A #StmManager
 * @transfer: A #StmTransfer
 * 
 * Remove transfer form manager.
 */
void
stm_manager_remove_transfer (StmManager *self, StmTransfer *transfer)
{
	StmManagerPrivate *priv = self->priv;

//	glibcurl_remove (_stm_transfer_get_handle (transfer));
	GList *node = g_list_find (priv->transfers, transfer);
	if (node != NULL) {
		priv->transfers = g_list_delete_link (priv->transfers, node);
		g_signal_emit (self, signals[TRANSFER_REMOVED], 0, transfer);
		g_object_unref (transfer);
//		g_print ("Removing transfer, left %u references\n", G_OBJECT (transfer)->ref_count);
	} else {
		g_printerr ("Attempting to remove transfer which is not added!");
	}
//	priv->transfers = g_list_remove (priv->transfers, transfer);

}


/*
 * State saving/loading
 */

/**
 * stm_manager_load_state_start_element:
 * 
 * Callback called when opening tag is encountered while parsing
 * xml state file
 * 
 * @see_also #GParser
 */
static void
stm_manager_load_state_start_element (GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer             user_data,
                                      GError             **error)
{
	StmManager *self = STM_MANAGER (user_data);
//	StmManagerPrivate *priv = self->priv;

	if (strcmp (element_name, "transfer") == 0) {
		StmTransfer *transfer = _stm_transfer_from_xml (element_name,
                                                        attribute_names,
                                                        attribute_values);
		if (transfer != NULL) {
			stm_manager_add_transfer (self, transfer);
//			if (stm_transfer_get_state (transfer) == STM_TRANSFER_STATE_RUNNING)
//				stm_transfer_start (transfer);
			g_object_unref (transfer);
		}
	}
}

/**
 * stm_manager_load_state_end_element:
 * 
 * Callback called when closing tag is encountered while parsing
 * xml state file
 * 
 * @see_also #GParser
 */
static void
stm_manager_load_state_end_element (GMarkupParseContext *context,
                                    const gchar         *element_name,
                                    gpointer             user_data,
                                    GError             **error)
{
//	StmManager *self = STM_MANAGER (user_data);
//	StmManagerPrivate *priv = self->priv;
}


/**
 * stm_manager_load_state:
 * 
 * @self A #StmManager
 * @file_name State file name
 * 
 * Restore state from file. file_name should be file written with 
 * stm_manager_save_state(). All transfers will be resumed from
 * point at which they were when stm_manager_save_state() was
 * called.
 * 
 * @see_also stm_manager_save_state()
 * 
 * Returns TRUE on success, FALSE otherwise.
 */ 
gboolean
stm_manager_load_state (StmManager *self, const gchar *file_name)
{
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (file_name, FALSE);

//	StmManagerPrivate *priv = self->priv;
	
	FILE *f = fopen (file_name, "r");
	if (f == NULL) {
		return FALSE;
	}
	char buffer[BUFFER_SIZE];
	
	/* Setup parser */
	GMarkupParser parser;
	memset (&parser, 0, sizeof (parser));
	parser.start_element = stm_manager_load_state_start_element;
	parser.end_element = stm_manager_load_state_end_element;
	
	GMarkupParseContext *ctx;
	ctx = g_markup_parse_context_new (&parser, 0, self, NULL);
	
	while (! feof (f)) {
		gsize bytes_read = fread (buffer, 1, BUFFER_SIZE, f);
		if (! g_markup_parse_context_parse (ctx, buffer, bytes_read, NULL)) {
			break;
		}
	}
	fclose (f);
	g_markup_parse_context_end_parse (ctx, NULL);
	g_markup_parse_context_free (ctx);
	
	return TRUE;
}


/**
 * stm_manager_save_state:
 * 
 * @self A #StmManager
 * @file_name State file name
 * 
 * Save manager state to a file. This function saves manager state,
 * including transfer state, so that they can be resumed later. State
 * saved to file can be restored using stm_manager_load_state().
 * 
 * @see_also stm_manager_load_state()
 * 
 * Returns: TRUE on success, FALSE otherwise.
 */
gboolean
stm_manager_save_state (StmManager *self, const gchar *file_name)
{
	g_return_val_if_fail (self, FALSE);
	g_return_val_if_fail (file_name, FALSE);

	StmManagerPrivate *priv = self->priv;
	
	FILE *f = fopen (file_name, "w");
	if (f == NULL) {
		return FALSE;
	}
	
	fprintf (f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf (f, "<transfers>\n");

	GList *node;
	for (node = priv->transfers; node; node = node->next) {
		StmTransfer *transfer = node->data;
		
		char *xml = _stm_transfer_to_xml (transfer);
		fprintf (f, "%s\n", xml);
		g_free (xml);
	}	
	
	fprintf (f, "</transfers>\n");
	
	fclose (f);
	
	return TRUE;
}


/**
 * idle_write_state:
 *
 * @self A #StmManager
 *
 * Callback run once in a while, saving current transfers state
 * to a file, so that they can be restored.
 */
static gboolean
idle_write_state (StmManager *self)
{
	StmManagerPrivate *priv = self->priv;

	if (priv->state_file)
		stm_manager_save_state (self, priv->state_file);

	return TRUE;
}


/**
 * stm_manager_set_state_file:
 * 
 * @self A #StmManager
 * @file State file name
 *
 * Set state file name, so that state of transfer is periodically
 * written to that file
 */
void
stm_manager_set_state_file (StmManager *self,
                            const gchar *file)
{
	g_return_if_fail (self);

	StmManagerPrivate *priv = self->priv;

	if (priv->state_file) {
		g_free (priv->state_file);
		g_source_remove (priv->state_file_id);
	}

	if (file) {
		priv->state_file = g_strdup (file);
		priv->state_file_id = g_timeout_add (10000,
		                                     (GSourceFunc) idle_write_state,
						     self);
	}
}


/**
 * stm_manager_get_state_file:
 *
 * @self: A #StmManager
 *
 * Get state file name.
 *
 * @see_also stm_manager_save_state()
 * @see_also stm_manager_set_state_file()
 *
 * Returns: State file name. Return value is owned by STM and should not
 * be freed or modified.
 */
const gchar*
stm_manager_get_state_file (StmManager *self)
{
	g_return_val_if_fail (self, NULL);

	StmManagerPrivate *priv = self->priv;
	
	return priv->state_file;
}



/*
 * Tree model associated with this manager
 */

/**
 * stm_manager_tm_update_iter:
 * 
 * Update iterator contents in associated TreeModel
 */
static void
stm_manager_tm_update_iter (StmManager *self, StmTransfer *transfer, GtkTreeIter *iter)
{
	StmManagerPrivate *priv = self->priv;
	
	static const gchar *stock_ids[] = {
		GTK_STOCK_FILE,
		GTK_STOCK_MEDIA_PAUSE,
		GTK_STOCK_MEDIA_PLAY,
		GTK_STOCK_OK,
		GTK_STOCK_CANCEL
	};

	guint64 downloaded      = stm_transfer_get_downloaded (transfer);
	guint64 content_length  = stm_transfer_get_content_length (transfer);
	guint64 speed           = stm_transfer_get_speed (transfer);
	gint    state           = stm_transfer_get_state (transfer);
	
	gchar buf1[16];
	gchar buf2[16];
	gchar buf3[18];

	double r		= (content_length != 0) ? ((double)downloaded / content_length) : 0.0;
	int ratio		= (int) 100 * r;

	stm_format_size_buffer (speed, buf3, 16);
	g_strlcat (buf3, "/s", 18);

	gtk_list_store_set (GTK_LIST_STORE (priv->model), iter,
	                    STM_COLUMN_TRANSFER,   transfer,
			            STM_COLUMN_URI,        stm_transfer_get_uri (transfer),
			            STM_COLUMN_FILE,       stm_transfer_get_file_name (transfer),
			            STM_COLUMN_DOWNLOADED, stm_format_size_buffer (downloaded, buf1, 16),
			            STM_COLUMN_TOTAL,      stm_format_size_buffer (content_length, buf2, 16),
			            STM_COLUMN_SPEED,      buf3,
			            STM_COLUMN_PERCENT,    ratio,
			            STM_COLUMN_STOCK_ID,   stock_ids[state],
			            -1);
}


/**
 * stm_manager_tm_progress:
 * 
 * Callback called whenever associated #StmTransfer receives some data
 */
static void
stm_manager_tm_progress (StmTransfer *transfer, StmManager *self)
{
	StmManagerPrivate *priv = self->priv;

	GtkTreeIter iter;
	gtk_tree_model_get_iter_first (priv->model, &iter);
	do {
		StmTransfer *t;
		gtk_tree_model_get (priv->model, &iter, 0, &t, -1);
		if (t == transfer) {
			/* Right iter */
			stm_manager_tm_update_iter (self, transfer, &iter);
		}
		g_object_unref (t);
	} while (gtk_tree_model_iter_next (priv->model, &iter));

}


/**
 * stm_manager_tm_transfer_added:
 * 
 * Callback called when a new #StmTransfer is added to #StmManager.
 * 
 * Add new row to associated #GtkTreeModel and update its iterator.
 */
static void
stm_manager_tm_transfer_added (StmManager *self, StmTransfer *transfer, gpointer data)
{
	StmManagerPrivate *priv = self->priv;
	GtkTreeIter iter;

	gtk_list_store_append (GTK_LIST_STORE (priv->model), &iter);
	stm_manager_tm_update_iter (self, transfer, &iter);

	g_signal_connect (transfer, "progress",
	                  G_CALLBACK (stm_manager_tm_progress), self);
}


/**
 * stm_manager_tm_transfer_removed:
 * 
 * Callback called when #StmTransfer is removed from #StmManager
 * 
 * Remove row from associated #GtkTreeModel
 */
static void
stm_manager_tm_transfer_removed (StmManager *self, StmTransfer *transfer, gpointer data)
{
	StmManagerPrivate *priv = self->priv;
	g_print ("Transfer removed!\n");

	GtkTreeIter iter;
	gtk_tree_model_get_iter_first (priv->model, &iter);
	do {
		StmTransfer *t;
		gtk_tree_model_get (priv->model, &iter, 0, &t, -1);
		if (t == transfer) {
			/* Right iter */
			gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter);
			g_object_unref (t);
			return;
		}
		g_object_unref (t);
	} while (gtk_tree_model_iter_next (priv->model, &iter));
}


/**
 * stm_manager_get_tree_model:
 * 
 * @self A #StmManager
 * 
 * Get associated #GtkTreeModel.
 * 
 * Model will be created if necessary. There will always be one
 * instance of model per #StmManager. This model can be viewed with
 * #GtkTreeView or #GtkIconView.
 * 
 * Returns: A #GtkTreeModel.
 */
GtkTreeModel *
stm_manager_get_tree_model (StmManager *self)
{
	StmManagerPrivate *priv = self->priv;
	// TODO: count references!!!

	if (priv->model != NULL)
		return priv->model;

	GtkListStore *model = gtk_list_store_new (8,
			STM_TYPE_TRANSFER,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_INT,
			G_TYPE_STRING);

	priv->model = GTK_TREE_MODEL (model);


	g_signal_connect (self, "transfer-added",
	                  G_CALLBACK (stm_manager_tm_transfer_added), NULL);
	g_signal_connect (self, "transfer-removed",
	                  G_CALLBACK (stm_manager_tm_transfer_removed), NULL);

	GList *node;
	for (node = priv->transfers; node; node = node->next) {
		StmTransfer *transfer = node->data;

		stm_manager_tm_transfer_added (self, transfer, NULL);
	}

	return priv->model;
}

/* GObject implementation */

static void
stm_manager_init (StmManager *self)
{
	self->priv = STM_MANAGER_GET_PRIVATE (self);
	StmManagerPrivate *priv = self->priv;

	priv->transfers = NULL;
	priv->model = NULL;

	priv->disposed = FALSE;
}


static void
stm_manager_dispose (GObject *object)
{
	StmManager *self = (StmManager*) object;
	StmManagerPrivate *priv = self->priv;

	g_print ("stm_manager_dispose ()\n");
	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;

	if (priv->model) {
		g_object_unref (priv->model);
	}
	
	GList *node;
	for (node = priv->transfers; node; node = node->next) {
		StmTransfer *transfer = node->data;
		
		g_print ("Unreffing transfer, now %u\n", G_OBJECT(transfer)->ref_count);
		g_object_unref (transfer);
	}
	g_list_free (priv->transfers);
	stm_manager_set_state_file (self, NULL);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_manager_parent_class)->dispose (object);
}


static void
stm_manager_finalize (GObject *object)
{
	G_OBJECT_CLASS (stm_manager_parent_class)->finalize (object);
}

	
static void
stm_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_manager_class_init (StmManagerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_manager_get_property;
	gobject_class->set_property = stm_manager_set_property;
	gobject_class->dispose = stm_manager_dispose;
	gobject_class->finalize = stm_manager_finalize;

	g_type_class_add_private (klass, sizeof (StmManagerPrivate));
	
	signals[TRANSFER_ADDED] = g_signal_new ("transfer-added",
	                                  G_TYPE_FROM_CLASS (klass),
	                                  G_SIGNAL_RUN_LAST,
	                                  G_STRUCT_OFFSET (StmManagerClass, transfer_added),
	                                  NULL, NULL,
	                                  g_cclosure_marshal_VOID__OBJECT,
	                                  G_TYPE_NONE, 1,
	                                  STM_TYPE_TRANSFER);
	                                  
	signals[TRANSFER_REMOVED] = g_signal_new ("transfer-removed",
	                                  G_TYPE_FROM_CLASS (klass),
	                                  G_SIGNAL_RUN_LAST,
	                                  G_STRUCT_OFFSET (StmManagerClass, transfer_removed),
	                                  NULL, NULL,
	                                  g_cclosure_marshal_VOID__OBJECT,
	                                  G_TYPE_NONE, 1,
	                                  STM_TYPE_TRANSFER);
	                                  

	
	/* Initialize cURL */
	curl_global_init (CURL_GLOBAL_ALL);
	glibcurl_init ();
}

