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

#ifndef __STM_MANAGER_H__
#define __STM_MANAGER_H__

/* Includes here */
#include <glib.h>
#include <gtk/gtktreemodel.h>
#include "stm.h"
#include "stm-transfer.h"


G_BEGIN_DECLS

#define STM_TYPE_MANAGER \
	(stm_manager_get_type ())
#define STM_MANAGER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), STM_TYPE_MANAGER, StmManager))
#define STM_MANAGER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), STM_TYPE_MANAGER, StmManagerClass))
#define STM_IS_MANAGER(obj) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), STM_TYPE_MANAGER))
#define STM_MANAGER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), STM_TYPE_MANAGER, StmManagerClass))


typedef struct _StmManager		StmManager;
typedef struct _StmManagerPrivate		StmManagerPrivate;
typedef struct _StmManagerClass		StmManagerClass;


typedef enum {
	STM_COLUMN_TRANSFER,
	STM_COLUMN_URI,
	STM_COLUMN_FILE,
	STM_COLUMN_DOWNLOADED,
	STM_COLUMN_TOTAL,
	STM_COLUMN_SPEED,
	STM_COLUMN_PERCENT,
	STM_COLUMN_STOCK_ID
} StmManagerColumn;

struct _StmManager{
	GObject		parent;
	StmManagerPrivate	*priv;
};

struct _StmManagerClass
{
	GObjectClass		parent;

	/* Signals */
	void				(*transfer_added) 	(StmManager *self, StmTransfer *transfer);
	void				(*transfer_removed)	(StmManager *self, StmTransfer *transfer);
};



GType
stm_manager_get_type				(void);

StmManager*
stm_manager_new				(void);


void
stm_manager_add_transfer (StmManager *self, StmTransfer *transfer);

void
stm_manager_remove_transfer (StmManager *self, StmTransfer *transfer);



gboolean
stm_manager_load_state (StmManager *self, const gchar *file_name);

gboolean
stm_manager_save_state (StmManager *self, const gchar *file_name);



GtkTreeModel *
stm_manager_get_tree_model (StmManager *self);


void
stm_manager_set_state_file (StmManager *self,
                            const gchar *file);

const gchar*
stm_manager_get_state_file (StmManager *self);


G_END_DECLS

#endif
