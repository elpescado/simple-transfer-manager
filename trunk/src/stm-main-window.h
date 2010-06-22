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

#ifndef __STM_MAIN_WINDOW_H__
#define __STM_MAIN_WINDOW_H__

/* Includes here */
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include "stm-manager.h"


G_BEGIN_DECLS

#define STM_TYPE_MAIN_WINDOW \
	(stm_main_window_get_type ())
#define STM_MAIN_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), STM_TYPE_MAIN_WINDOW, StmMainWindow))
#define STM_MAIN_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), STM_TYPE_MAIN_WINDOW, StmMainWindowClass))
#define STM_IS_MAIN_WINDOW(obj) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), STM_TYPE_MAIN_WINDOW))
#define STM_MAIN_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), STM_TYPE_MAIN_WINDOW, StmMainWindowClass))


typedef struct _StmMainWindow		StmMainWindow;
typedef struct _StmMainWindowPrivate		StmMainWindowPrivate;
typedef struct _StmMainWindowClass		StmMainWindowClass;

struct _StmMainWindow{
	GtkWindow		parent;
	StmMainWindowPrivate	*priv;
};

struct _StmMainWindowClass
{
	GtkWindowClass		parent;

	/* Signals */
};



GType
stm_main_window_get_type				(void);

GtkWidget*
stm_main_window_new				(StmManager *manager);


G_END_DECLS

#endif
