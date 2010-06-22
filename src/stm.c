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

#include "stm.h"
#include <gtk/gtk.h>
#include <string.h>


static gchar *units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};


/**
 * stm_format_size:
 * 
 * @size: Size (in bytes) to be formatted
 * 
 * Formats size for display.
 * 
 * This function uses SI binary prefixes (Ki, Mi and so on) to indicate
 * that they are 2-based.
 * 
 * Returns: Newly allocated string to be freed with g_free() when no
 * longer used.
 */
gchar *
stm_format_size (guint64 size)
{
	gint unit = 0;
	while (size >= 1024) {
		size /= 1024;
		unit++;
	}
	gint s = (gint) size;
	return g_strdup_printf ("%d %s", s, units[unit]);
}


/**
 * stm_format_size_buffer:
 * 
 * @size: Size (in bytes) to be formatted
 * @buffer Character buffer in which formatted string will be stored
 * @buffer_size size of buffer
 * 
 * Formats size for display.
 * 
 * This function uses SI binary prefixes (Ki, Mi and so on) to indicate
 * that they are 2-based.
 * 
 * Returns: Pointer to @buffer.
 */
gchar *
stm_format_size_buffer (guint64 size, char *buffer, gsize buffer_size)
{
	gint unit = 0;
	while (size >= 1024) {
		size /= 1024;
		unit++;
	}
	gint s = (gint) size;
	snprintf (buffer, buffer_size, "%d %s", s, units[unit]);
	return buffer;
}


/**
 * stm_format_time:
 * 
 * @time: Number of seconds
 * @buffer Character buffer in which formatted string will be stored
 * @buffer_size size of buffer
 * 
 * Formats number of seconds as mm:ss or h:mm:ss. Hour part is optional
 * and will be present only if @time > 3600.
 * 
 * Returns: Pointer to @buffer
 */
gchar *
stm_format_time_buffer (guint64 time, char *buffer, gsize buffer_size)
{
	guint seconds = time % 60;
	guint minutes = (time / 60) % 60;
	guint hours = (time / 3600);
	
	if (hours > 0) {
		snprintf (buffer, buffer_size, "%02u:%02u:%02u",
		          hours, minutes, seconds);
	} else {
		snprintf (buffer, buffer_size, "%02u:%02u",
		          minutes, seconds);
	}
	return buffer;
}


/**
 * stm_basename:
 *
 * @file: File name
 *
 * Returns file component of a path.
 *
 * Returns: File component of a path
 **/
const gchar *
stm_basename (const gchar *file)
{
	gchar *file_name = strrchr (file, '/');
	if (file_name != NULL)
		return file_name+1;
	else
		return file;
}


/**
 * stm_find_user_file:
 *
 * @file: File name
 *
 * Find file in user's configuration directory
 */
gchar *
stm_find_user_file (const gchar *file)
{
	gchar *dir = g_build_filename (g_get_user_config_dir (), "stm", NULL);
	if (! g_file_test (dir, G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents (dir, 0700);
	g_free (dir);
	return g_build_filename (g_get_user_config_dir (),
	                         "stm",
				 file,
				 NULL);
}
