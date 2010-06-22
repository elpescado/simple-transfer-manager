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

#include <string.h>
#include <gtk/gtk.h>
#include "stm-panel.h"
#include "stm-transfer.h"
#include "stm-manager.h"
#include "stm-transfer-window.h"
#include "stm-new-transfer-window.h"


G_DEFINE_TYPE (StmPanel, stm_panel, GTK_TYPE_VBOX)

struct _StmPanelPrivate
{
	/* Private members go here */
	StmManager	*manager;			/* Transfers manager */
	GtkActionGroup	*action_group;	/* Action group */
	GtkUIManager	*ui_manager;
	GtkWidget	*toolbar;			/* Tool bar */
	GtkWidget	*scrolled_window;	/* Scrolled window holding tree view */
	GtkWidget	*tree_view;			/* Transfers list */
	GtkWidget	*search_entry;		/* Search entry */
	
	GtkTreeModel *filter;

	gboolean disposed;
};


#define STM_PANEL_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_PANEL, StmPanelPrivate))

static gboolean
stm_panel_filter_function                               (GtkTreeModel *model,
                                                         GtkTreeIter *iter,
                                                         gpointer data);



GtkWidget*
stm_panel_new (void)
{
	StmPanel *self = g_object_new (STM_TYPE_PANEL, NULL);
	return GTK_WIDGET (self);
}


void
stm_panel_set_manager (StmPanel *self, StmManager *manager)
{
	StmPanelPrivate *priv = self->priv;

	GtkTreeModel *model = stm_manager_get_tree_model (manager);
	GtkTreeModel *filter = gtk_tree_model_filter_new (model, NULL);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter),
	                                        stm_panel_filter_function,
	                                        self,
	                                        NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), filter);
	priv->manager = g_object_ref (manager);
	priv->filter = g_object_ref (filter);
}

static gboolean
stm_panel_filter_function                               (GtkTreeModel *model,
                                                         GtkTreeIter *iter,
                                                         gpointer data)
{
	StmPanel *self = STM_PANEL (data);
	StmPanelPrivate *priv = self->priv;
	
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (priv->search_entry));
	if (*text == '\0')
		return TRUE;

	gchar *file;
	gtk_tree_model_get (model, iter, STM_COLUMN_FILE, &file, -1);
	gchar *found = strstr (file, text);
	g_free (file);
	
	return found != NULL;
}


static void
stm_panel_search_entry_changed                         (GtkEditable *editable,
                                                        gpointer data)
{
	StmPanel *self = STM_PANEL (data);
	StmPanelPrivate *priv = self->priv;
	
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
}


/*
 *  Actions
 */
static StmTransfer *
stm_panel_get_selected_transfer (StmPanel *self)
{
	StmPanelPrivate *priv = self->priv;
	GtkTreeIter iter;
	GtkTreeModel *model;
	StmTransfer *transfer;
	
	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
	if (gtk_tree_selection_get_selected (select, &model, &iter)) {
		gtk_tree_model_get (model, &iter, STM_COLUMN_TRANSFER, &transfer, -1);
		g_object_unref (transfer); // TODO: ???
		return transfer;
	}
	return NULL;
}


static void
stm_panel_action_set_sensitive (StmPanel *self, const gchar *action_name, gboolean sensitive)
{
	StmPanelPrivate *priv = self->priv;
	GtkAction *action = gtk_action_group_get_action (priv->action_group, action_name);
	gtk_action_set_sensitive (action, sensitive);
}


/**
 * update_actions:
 * 
 * Update sensitivity of toolbar actions
 */
static void
_update_actions (StmPanel *self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	StmTransferState state = transfer ? stm_transfer_get_state (transfer) : STM_TRANSFER_STATE_NONE;

	stm_panel_action_set_sensitive (self, "TransferProperties", transfer != NULL);
	
	stm_panel_action_set_sensitive (self, "TransferOpen", state == STM_TRANSFER_STATE_FINISHED);
	stm_panel_action_set_sensitive (self, "TransferOpenDir", state == STM_TRANSFER_STATE_FINISHED);
	
	stm_panel_action_set_sensitive (self, "TransferStart", state == STM_TRANSFER_STATE_STOPPED || state == STM_TRANSFER_STATE_ERROR);
	stm_panel_action_set_sensitive (self, "TransferStop", state == STM_TRANSFER_STATE_RUNNING);
	stm_panel_action_set_sensitive (self, "TransferDelete", transfer != NULL);
}

static void
stm_panel_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	StmPanel *self = STM_PANEL (data);
//	StmPanelPrivate *priv = self->priv;
	
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
	} else {
	}
	
	_update_actions (self);
}

/* Callbacks */

static gboolean
stm_panel_button_pressed (GtkTreeSelection *selection, GdkEventButton *event, gpointer data)
{
	StmPanel *self = STM_PANEL (data);
	StmPanelPrivate *priv = self->priv;

	if (event->button == 3) {
		GtkWidget *menu = gtk_ui_manager_get_widget (priv->ui_manager, "/popup");
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
		                NULL, NULL,
				event->button, event->time);
	}
	return FALSE;
}


static void
on_new_transfer_response                               (GtkDialog *dialog,
                                                        gint       arg1,
                                                        gpointer   user_data)
{
	if (arg1 != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (dialog));
		return;
	}
	
	gtk_widget_hide (GTK_WIDGET (dialog));
	
	StmPanel *self = STM_PANEL (user_data);
	StmPanelPrivate *priv = self->priv;
	
	const gchar *uri = stm_new_transfer_window_get_uri (
	                  STM_NEW_TRANSFER_WINDOW (dialog));
	const gchar *dest = stm_new_transfer_window_get_destination (
	                   STM_NEW_TRANSFER_WINDOW (dialog));
	gboolean start = stm_new_transfer_window_get_auto_start (
	                 STM_NEW_TRANSFER_WINDOW (dialog));
	
	StmTransfer *xfer = stm_transfer_new (uri, dest);
	stm_manager_add_transfer (priv->manager, xfer);
	if (start) {
		stm_transfer_start (xfer);
	}
	
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
on_action_transfer_new (GtkAction*     action,
                        StmPanel* self)
{
	GtkWidget *win = stm_new_transfer_window_new ();
	g_signal_connect (G_OBJECT (win), "response",
	                  G_CALLBACK (on_new_transfer_response), self);
	gtk_widget_show (win);
	
}


static void
on_action_transfer_properties (GtkAction*     action,
                               StmPanel* self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		GtkWidget *win;
		win = stm_transfer_window_new ();
		stm_transfer_window_set_transfer (STM_TRANSFER_WINDOW (win), transfer);
		gtk_widget_show (win);
	}
}


static void
on_action_transfer_open_file (GtkAction*     action,
                              StmPanel* self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		stm_transfer_open_file (transfer);
	}
}


static void
on_action_transfer_open_dir (GtkAction*     action,
                             StmPanel* self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		stm_transfer_open_directory (transfer);
	}
}

static void
on_action_transfer_start (GtkAction*     action,
                          StmPanel* self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		stm_transfer_start (transfer);
		_update_actions (self);
	}
}


static void
on_action_transfer_stop (GtkAction*     action,
                         StmPanel* self)
{
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		stm_transfer_stop (transfer);
		_update_actions (self);
	}
}


static void
on_action_transfer_delete (GtkAction*     action,
                           StmPanel* self)
{
	StmPanelPrivate *priv = self->priv;
	
	StmTransfer *transfer = stm_panel_get_selected_transfer (self);
	if (transfer) {
		stm_manager_remove_transfer (priv->manager, transfer);
	}
}


/* User interface */

static const GtkActionEntry entries[] = {
	{ "TransferNew", GTK_STOCK_NEW, N_("New transfer"), NULL,
		N_("Add a new transfer"), G_CALLBACK (on_action_transfer_new) },
	{ "TransferProperties", GTK_STOCK_PROPERTIES, NULL, NULL,
		N_("Transfer properties"), G_CALLBACK (on_action_transfer_properties) },
		
	{ "TransferOpen", GTK_STOCK_OPEN, N_("Open file"), NULL,
		N_("Open file"), G_CALLBACK (on_action_transfer_open_file) },
	{ "TransferOpenDir", GTK_STOCK_DIRECTORY, N_("Open directory"), NULL,
		N_("Open file parent directory"), G_CALLBACK (on_action_transfer_open_dir) },

	{ "TransferStart", GTK_STOCK_MEDIA_PLAY, N_("Start"), NULL,
		N_("Start transfer"), G_CALLBACK (on_action_transfer_start) },
	{ "TransferStop", GTK_STOCK_MEDIA_PAUSE, N_("Stop"), NULL,
		N_("Pause transfer"), G_CALLBACK (on_action_transfer_stop) },
	{ "TransferDelete", GTK_STOCK_DELETE, NULL, NULL,
		N_("Delete transfer"), G_CALLBACK (on_action_transfer_delete) },
};
static const guint entries_n = G_N_ELEMENTS (entries);

/**
 * \brief User interface description
 **/
static const gchar *ui_markup = 
	"<ui>"
		/* Main toolbar */
		"<toolbar name='toolbar'>"
			"<toolitem action='TransferNew'/>"
			"<toolitem action='TransferProperties'/>"
			"<separator/>"
			"<toolitem action='TransferOpen'/>"
			"<toolitem action='TransferOpenDir'/>"
			"<separator/>"
			"<toolitem action='TransferStart'/>"
			"<toolitem action='TransferStop'/>"
			"<toolitem action='TransferDelete'/>"
		"</toolbar>"

		/* Popup menu */
		"<popup name='popup'>"
			"<menuitem action='TransferProperties'/>"
			"<separator/>"
			"<menuitem action='TransferOpen'/>"
			"<menuitem action='TransferOpenDir'/>"
			"<separator/>"
			"<menuitem action='TransferStart'/>"
			"<menuitem action='TransferStop'/>"
			"<menuitem action='TransferDelete'/>"
		"</popup>"
	"</ui>";


static void
stm_panel_ui_toolbar (StmPanel *self)
{
	StmPanelPrivate *priv = self->priv;

	GtkActionGroup *group = gtk_action_group_new ("StmPanel");
//	gtk_action_group_set_translation_domain (group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (group, entries, entries_n, self);
	priv->ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (priv->ui_manager, group, 0);
	gtk_ui_manager_add_ui_from_string(priv->ui_manager, ui_markup, -1, NULL);
	priv->toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/toolbar");
	gtk_widget_show (priv->toolbar);
	gtk_box_pack_start (GTK_BOX (self), priv->toolbar, FALSE, FALSE, 0);
	priv->action_group = group;
	
	/* Spacer */
	GtkToolItem *separator = gtk_separator_tool_item_new ();
	gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (separator), FALSE);
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (separator), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (separator), -1);
	gtk_widget_show (GTK_WIDGET (separator));
	
	/* Search entry */
	GtkToolItem *item = gtk_tool_item_new ();
	GtkWidget *entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (entry), "changed",
	                  G_CALLBACK (stm_panel_search_entry_changed), self);
	gtk_container_add (GTK_CONTAINER (item), entry);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (item), -1);
	gtk_widget_show (GTK_WIDGET (item));
	gtk_widget_show (entry);
	priv->search_entry = entry;
}

static void
stm_panel_ui (StmPanel *self)
{
	StmPanelPrivate *priv = self->priv;
	
	stm_panel_ui_toolbar (self);

	priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->scrolled_window),
	                                GTK_POLICY_AUTOMATIC,
	                                GTK_POLICY_AUTOMATIC);
				       
	priv->tree_view = gtk_tree_view_new ();
	
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

/*
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("File", renderer,
	                                                   "text", 2,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);
*/
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("File"));
	gtk_tree_view_column_set_expand (column, TRUE);
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column,
	                                    renderer,
	                                    "stock-id",
	                                    STM_COLUMN_STOCK_ID);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column,
	                                    renderer,
	                                    "text",
	                                    STM_COLUMN_FILE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);
	
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Downloaded", renderer,
	                                                   "text", 3,
	                                                   NULL);
	g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_RIGHT,
	                                   "xalign", 1.0f,
	                                   NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Total", renderer,
	                                                   "text", 4,
	                                                   NULL);
	g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_RIGHT,
	                                   "xalign", 1.0f,
	                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);

	renderer = gtk_cell_renderer_progress_new ();
	column = gtk_tree_view_column_new_with_attributes ("Progress", renderer,
	                                                   "value", 6,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Speed", renderer,
	                                                   "text", 5,
	                                                   NULL);
	g_object_set (G_OBJECT (renderer), "alignment", PANGO_ALIGN_RIGHT,
	                                   "xalign", 1.0f,
	                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree_view), column);


	gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->tree_view);
	gtk_box_pack_start (GTK_BOX (self), priv->scrolled_window, TRUE, TRUE, 0);

	GtkTreeSelection *select;
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (select), "changed",
	                  G_CALLBACK (stm_panel_selection_changed), self);
	_update_actions (self);
	g_signal_connect (G_OBJECT (priv->tree_view), "button-press-event",
	                  G_CALLBACK (stm_panel_button_pressed), self);

	gtk_widget_show (priv->tree_view);
	gtk_widget_show (priv->scrolled_window);
}


static void
stm_panel_init (StmPanel *self)
{
	self->priv = STM_PANEL_GET_PRIVATE (self);
	StmPanelPrivate *priv = self->priv;

	priv->disposed = FALSE;
	priv->manager = NULL;

	stm_panel_ui (self);
}


static void
stm_panel_dispose (GObject *object)
{
	StmPanel *self = (StmPanel*) object;
	StmPanelPrivate *priv = self->priv;


	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;
	
	if (priv->manager)
		g_object_unref (priv->manager);


	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_panel_parent_class)->dispose (object);
}


static void
stm_panel_finalize (GObject *object)
{
	G_OBJECT_CLASS (stm_panel_parent_class)->finalize (object);
}

	
static void
stm_panel_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
//	StmPanel* self = STM_PANEL (object);
//	StmPanelPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_panel_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
//	StmPanel* self = STM_PANEL (object);
//	StmPanelPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_panel_class_init (StmPanelClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_panel_get_property;
	gobject_class->set_property = stm_panel_set_property;
	gobject_class->dispose = stm_panel_dispose;
	gobject_class->finalize = stm_panel_finalize;

	g_type_class_add_private (klass, sizeof (StmPanelPrivate));
}

