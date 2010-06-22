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

#ifndef __STM_NEW_TRANSFER_WINDOW_H__
#define __STM_NEW_TRANSFER_WINDOW_H__

/* Includes here */
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>


G_BEGIN_DECLS

#define STM_TYPE_NEW_TRANSFER_WINDOW \
	(stm_new_transfer_window_get_type ())
#define STM_NEW_TRANSFER_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), STM_TYPE_NEW_TRANSFER_WINDOW, StmNewTransferWindow))
#define STM_NEW_TRANSFER_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), STM_TYPE_NEW_TRANSFER_WINDOW, StmNewTransferWindowClass))
#define STM_IS_NEW_TRANSFER_WINDOW(obj) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), STM_TYPE_NEW_TRANSFER_WINDOW))
#define STM_NEW_TRANSFER_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), STM_TYPE_NEW_TRANSFER_WINDOW, StmNewTransferWindowClass))


typedef struct _StmNewTransferWindow		StmNewTransferWindow;
typedef struct _StmNewTransferWindowPrivate		StmNewTransferWindowPrivate;
typedef struct _StmNewTransferWindowClass		StmNewTransferWindowClass;

struct _StmNewTransferWindow{
	GtkDialog		parent;
	StmNewTransferWindowPrivate	*priv;
};

struct _StmNewTransferWindowClass
{
	GtkDialogClass		parent;

	/* Signals */
};



GType
stm_new_transfer_window_get_type           (void);

GtkWidget*
stm_new_transfer_window_new				   (void);

const gchar *
stm_new_transfer_window_get_uri            (StmNewTransferWindow *self);

const gchar *
stm_new_transfer_window_get_destination    (StmNewTransferWindow *self);

gboolean
stm_new_transfer_window_get_auto_start     (StmNewTransferWindow *self);


G_END_DECLS

#endif
