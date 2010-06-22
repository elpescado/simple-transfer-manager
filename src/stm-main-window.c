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
#include "stm-main-window.h"
#include "stm-panel.h"


G_DEFINE_TYPE (StmMainWindow, stm_main_window, GTK_TYPE_WINDOW)

struct _StmMainWindowPrivate
{
	/* Private members go here */
	GtkWidget	*panel;	// panel
	
	StmManager	*manager;

	gboolean disposed;
};


#define STM_MAIN_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_MAIN_WINDOW, StmMainWindowPrivate))


GtkWidget*
stm_main_window_new (StmManager *manager)
{
	StmMainWindow *self = g_object_new (STM_TYPE_MAIN_WINDOW, NULL);
	StmMainWindowPrivate *priv = self->priv;
	
	priv->manager = g_object_ref (manager);
	stm_panel_set_manager (STM_PANEL (priv->panel), priv->manager);

	return GTK_WIDGET (self);
}


/**
 * stm_main_window_ui:
 * 
 * Set up user interface
 */
static void
stm_main_window_ui (StmMainWindow *self)
{
	StmMainWindowPrivate *priv = self->priv;

	gtk_window_set_title (GTK_WINDOW (self), "Simple Transfer Manager");
	gtk_window_set_default_size (GTK_WINDOW (self), 460, 240);
	
	priv->panel = stm_panel_new ();
	gtk_container_add (GTK_CONTAINER (self), priv->panel);
	gtk_widget_show (priv->panel);
}


/* GObject implementation */

static void
stm_main_window_init (StmMainWindow *self)
{
	self->priv = STM_MAIN_WINDOW_GET_PRIVATE (self);
	StmMainWindowPrivate *priv = self->priv;

	priv->disposed = FALSE;
	
	stm_main_window_ui (self);
}


static void
stm_main_window_dispose (GObject *object)
{
	StmMainWindow *self = (StmMainWindow*) object;
	StmMainWindowPrivate *priv = self->priv;


	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;

	g_object_unref (priv->manager);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_main_window_parent_class)->dispose (object);
}


static void
stm_main_window_finalize (GObject *object)
{
	G_OBJECT_CLASS (stm_main_window_parent_class)->finalize (object);
}

	
static void
stm_main_window_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
//	StmMainWindow* self = STM_MAIN_WINDOW (object);
//	StmMainWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_main_window_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
//	StmMainWindow* self = STM_MAIN_WINDOW (object);
//	StmMainWindowPrivate* priv = self->priv;

	switch (property_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_main_window_class_init (StmMainWindowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_main_window_get_property;
	gobject_class->set_property = stm_main_window_set_property;
	gobject_class->dispose = stm_main_window_dispose;
	gobject_class->finalize = stm_main_window_finalize;

	g_type_class_add_private (klass, sizeof (StmMainWindowPrivate));
}

