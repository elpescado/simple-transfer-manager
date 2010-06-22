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

#ifndef __STM_TRANSFER_H__
#define __STM_TRANSFER_H__

/* Includes here */
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include "stm.h"


G_BEGIN_DECLS

#define STM_TYPE_TRANSFER \
	(stm_transfer_get_type ())
#define STM_TRANSFER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), STM_TYPE_TRANSFER, StmTransfer))
#define STM_TRANSFER_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), STM_TYPE_TRANSFER, StmTransferClass))
#define STM_IS_TRANSFER(obj) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), STM_TYPE_TRANSFER))
#define STM_TRANSFER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), STM_TYPE_TRANSFER, StmTransferClass))


typedef struct _StmTransfer		StmTransfer;
typedef struct _StmTransferPrivate		StmTransferPrivate;
typedef struct _StmTransferClass		StmTransferClass;

struct _StmTransfer{
	GObject		parent;
	StmTransferPrivate	*priv;
};

struct _StmTransferClass
{
	GObjectClass		parent;

	/* Signals */
	void				(*progress) (StmTransfer *self);
	void				(*started)	(StmTransfer *self);
	void				(*finished)	(StmTransfer *self);
};

typedef enum {
	STM_TRANSFER_STATE_NONE,
	STM_TRANSFER_STATE_STOPPED,
	STM_TRANSFER_STATE_RUNNING,
	STM_TRANSFER_STATE_FINISHED,
	STM_TRANSFER_STATE_ERROR
} StmTransferState;

GType
stm_transfer_get_type				(void);

StmTransfer*
stm_transfer_new				(const gchar* uri, const gchar *file);


void
stm_transfer_start (StmTransfer *self);

void
stm_transfer_stop (StmTransfer *self);

/*
void
stm_transfer_open (StmTransfer *self);
*/
void *
stm_transfer_get_handle (StmTransfer *self);

const gchar *
stm_transfer_get_md5 (StmTransfer *self);

void
stm_transfer_open_file (StmTransfer *self);

void
stm_transfer_open_directory (StmTransfer *self);


StmTransferState
stm_transfer_get_state                             (StmTransfer *self);

const gchar*
stm_transfer_get_uri                               (StmTransfer *self);

const gchar*
stm_transfer_get_file                              (StmTransfer *self);

const gchar*
stm_transfer_get_file_name                         (StmTransfer *self);

guint64
stm_transfer_get_content_length                    (StmTransfer *self);

guint64
stm_transfer_get_downloaded                        (StmTransfer *self);

guint64
stm_transfer_get_speed                             (StmTransfer *self);

guint64
stm_transfer_get_total_time                        (StmTransfer *self);

guint64
stm_transfer_get_eta                               (StmTransfer *self);


G_END_DECLS

#endif
