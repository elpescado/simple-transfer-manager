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

#ifndef __STM_H__
#define __STM_H__

#include <glib.h>

#define STM_DEBUG_PROP(name, val) (g_printerr (" * setting '%s' to '%s'\n", name, val))
#define _(a) (a)
#define N_(a) (a)


gchar *
stm_format_size (guint64 size);


gchar *
stm_format_size_buffer (guint64 size, char *buffer, gsize buffer_size);


gchar *
stm_format_time_buffer (guint64 time, char *buffer, gsize buffer_size);


const gchar *
stm_basename (const gchar *file);


gchar *
stm_find_user_file (const gchar *file);

#endif
