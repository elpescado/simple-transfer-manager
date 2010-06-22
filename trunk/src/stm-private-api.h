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

#ifndef __STM_PRIVATE_API_H__
#define __STM_PRIVATE_API_H__

#define BUFFER_SIZE 4096


/* Transfers */

gchar *
_stm_transfer_to_xml (StmTransfer *transfer);

StmTransfer *
_stm_transfer_from_xml (const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values);

#endif
