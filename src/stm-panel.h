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

#ifndef __STM_PANEL_H__
#define __STM_PANEL_H__

/* Includes here */
#include <gtk/gtk.h>
#include "stm-manager.h"
#include "stm-transfer.h"


G_BEGIN_DECLS

#define STM_TYPE_PANEL \
	(stm_panel_get_type ())
#define STM_PANEL(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), STM_TYPE_PANEL, StmPanel))
#define STM_PANEL_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), STM_TYPE_PANEL, StmPanelClass))
#define STM_IS_PANEL(obj) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), STM_TYPE_PANEL))
#define STM_PANEL_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), STM_TYPE_PANEL, StmPanelClass))


typedef struct _StmPanel		StmPanel;
typedef struct _StmPanelPrivate		StmPanelPrivate;
typedef struct _StmPanelClass		StmPanelClass;

struct _StmPanel{
	GtkVBox		parent;
	StmPanelPrivate	*priv;
};

struct _StmPanelClass
{
	GtkVBoxClass		parent;

	/* Signals */
};



GType
stm_panel_get_type				(void);

GtkWidget*
stm_panel_new					(void);

void
stm_panel_set_manager 				(StmPanel *self,
						 StmManager *manager);

G_END_DECLS

#endif
