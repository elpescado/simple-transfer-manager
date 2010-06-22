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
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "stm-transfer.h"
#include "glibcurl.h"

#ifdef HAVE_CRYPTO
#include <openssl/md5.h>
#endif

gint n_updates = 0;

G_DEFINE_TYPE (StmTransfer, stm_transfer, G_TYPE_OBJECT)

struct _StmTransferPrivate
{
	/* Private members go here */
	StmTransferState state; // Transfer state
	
	gchar		*uri;		// Download URI
	gchar		*file;		// Destination file path
	const gchar	*file_name;	// Destination file name
	gchar		*path;		// Destination directory
	gchar		*tmpfile;	// Destination temporary download file name
	gboolean	output_is_dir;
	
	CURL		*curl;		// Curl handle
//	GIOChannel 	*io;		// I/O Channel
	FILE 		*out;		// Output file
	
	guint64		 length; 	// transfer length
	guint64 	 completed;	// successfully transferred bytes
	gfloat		 speed;		// dl speed
	
	guint64		 last_bytes;// last progress callback
	time_t		 last_time;	// last progress callback
	
	gchar		*error_buffer;	// buffer for error msg
	gchar		*error_msg;		// last error message

#ifdef HAVE_CRYPTO
	MD5_CTX		*md5_ctx;	// md5 computation context
	gchar		*md5;		// computed md5 chcecksum
#endif

	gboolean 	 disposed;
	int i;
};


#define STM_TRANSFER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
	STM_TYPE_TRANSFER, StmTransferPrivate))


/* Properties */
enum {
	PROP_0,
	
	PROP_URI,
	PROP_FILE,
	
	PROP_CONTENT_LENGTH,
	PROP_COMPLETED
};


/* Signals */
enum {
	PROGRESS,
	FINISHED,
	STARTED,
	
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


static GHashTable *all_transfers = NULL;


static size_t
stm_transfer_write_data (void *buffer, size_t size, size_t nmemb, void *userp);

static int
stm_transfer_progress_callback (void *clientp,
                                double dltotal,
                                double dlnow,
                                double ultotal,
                                double ulnow);
static void
_stm_transfer_set_state                            (StmTransfer *self,
                                                    StmTransferState state);



/**
 * stm_transfer_new:
 * 
 * @uri URI to download
 * @file File name to save downloaded data to
 * 
 * Create a new transfer. You may add it to transfer manager using
 * stm_manager_add_transfer(). Start transfer using
 * stm_transfer_start().
 */
StmTransfer*
stm_transfer_new (const gchar* uri, const gchar *file)
{
	StmTransfer *self = g_object_new (STM_TYPE_TRANSFER,
	                                  "uri",  uri,
	                                  "file", file,
	                                  NULL);
	StmTransferPrivate *priv = self->priv;

	priv->length = 0;
	priv->completed = 0;
	priv->state = STM_TRANSFER_STATE_STOPPED;
	priv->output_is_dir = FALSE;

	if (g_file_test (file, G_FILE_TEST_IS_DIR)) {
		priv->output_is_dir = TRUE;
		const gchar *uri_file = stm_basename (uri);
		if (!uri_file || !*uri_file) uri_file = "index.html";
		g_print ("uri_file = '%s'\n", uri_file);
		gchar *new_file = g_strdup_printf ("%s/%s", file, uri_file);
		g_free (priv->file);
		priv->file = new_file;
	}

	/* Get output file name */
	priv->file_name = stm_basename (priv->file);
	g_print ("priv->file = '%s'\n", priv->file);
//	gchar *file_name = strrchr (priv->file, '/');
//	if (file_name != NULL)
//		priv->file_name = file_name+1;
//	else
//		priv->file_name = priv->file;

//	stm_transfer_open (self);
	return self;
}


/**
 * stm_transfer_open:
 * 
 * #self: A #StmTransfer
 * 
 * Open transfer, set up network connections and open files.
 * This function effectively starts the transfer
 */
static void
stm_transfer_open (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	
	/* Set up destination */
/*	GError *error = NULL;
	priv->io = g_io_channel_new_file (file, "w", &error);
	if (priv->io == NULL) {
		g_printerr ("Unable open %s for writing: %s\n", file, error->message);
		g_error_free (error);
	}
*/
	g_print ("sizeof (off_t) = %d\n", sizeof (off_t));
	if (priv->completed > 0)
		priv->out = fopen (priv->file, "r+");
	else
		priv->out = fopen (priv->file, "w");
		
	
	if (priv->out == NULL) {
		g_printerr ("Unable open %s for writing\n", priv->file);
	}
	/* Prepare for resume */
	if (priv->completed >= 0) {
		if (fseeko (priv->out, (off_t) priv->completed, SEEK_SET) == -1) {
			g_printerr ("Unable to seek %s\n", priv->file);
		}
	}


	/* Set up CURL */
	priv->curl = curl_easy_init ();
	curl_easy_setopt (priv->curl, CURLOPT_URL, priv->uri);
	curl_easy_setopt (priv->curl, CURLOPT_WRITEFUNCTION, stm_transfer_write_data);
	curl_easy_setopt (priv->curl, CURLOPT_WRITEDATA, self);
	curl_easy_setopt (priv->curl, CURLOPT_NOPROGRESS, FALSE);
	curl_easy_setopt (priv->curl, CURLOPT_PROGRESSFUNCTION, stm_transfer_progress_callback);
	curl_easy_setopt (priv->curl, CURLOPT_PROGRESSDATA, self);
	curl_easy_setopt (priv->curl, CURLOPT_ERRORBUFFER, priv->error_buffer);
	curl_easy_setopt (priv->curl, CURLOPT_USERAGENT, "Simple Transfer Manager");
//	curl_easy_setopt (priv->curl, CURLOPT_MAX_RECV_SPEED_LARGE, limit64); // behaves strangely:/
//	curl_easy_setopt (priv->curl, CURLOPT_RESUME_FROM, 10L);	// TODO
	if (priv->completed > 0) {	
		curl_easy_setopt (priv->curl, CURLOPT_RESUME_FROM_LARGE, (off64_t) priv->completed); // requires off64_t
	}
//	curl_easy_setopt (priv->curl, CURLOPT_VERBOSE, 1);
	
	/* Register in global message dispatcher */
	g_hash_table_insert (all_transfers, priv->curl, self);
//	g_print ("Registering %p as %s\n", priv->curl, priv->file);
	
	glibcurl_add (priv->curl);
	_stm_transfer_set_state (self, STM_TRANSFER_STATE_RUNNING);
	
	priv->i = 0;
}


/**
 * stm_transfer_close:
 * 
 * Close a transfer. Close all files, and set state to STOPPED.
 */
static void
stm_transfer_close (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	
	glibcurl_remove (priv->curl);
	g_print ("Closing file %s", priv->file);
	
	fclose (priv->out);
	priv->out = NULL;
	_stm_transfer_set_state (self, STM_TRANSFER_STATE_STOPPED);
}


/**
 * stm_transfer_finish:
 * 
 * @self: A #StmTransfer
 * 
 * Finish transfer. This function cleans up, closes files, finishes
 * MD5 computation, and emits appropriate signals.
 * 
 * Should be called on end of transfer.
 */
static void
stm_transfer_finish (StmTransfer *self, int return_code)
{
	StmTransferPrivate *priv = self->priv;

	stm_transfer_close (self);
	if (return_code == 0) {
		_stm_transfer_set_state (self, STM_TRANSFER_STATE_FINISHED);
	} else {
		_stm_transfer_set_state (self, STM_TRANSFER_STATE_ERROR);
		priv->error_msg = g_strdup (priv->error_buffer);
		g_print ("Error code %d: %s\n", return_code, priv->error_msg);
	}

#ifdef HAVE_CRYPTO
	guchar md5[16];
	MD5_Final (md5, priv->md5_ctx);
	
	/* Get textual representation of MD5 checksum */
	gchar *buffer = g_new (gchar, 33);
	int i;
	for (i = 0; i < 16; i++) {
		snprintf (buffer+2*i, 3, "%02x", (int) md5[i]);
	}
	priv->md5 = buffer;
	g_print ("Finished, md5=%s\n", priv->md5);
#endif
	
	g_print ("finished, code=%d\n", return_code); // TODO
	g_signal_emit (self, signals[PROGRESS], 0);
	g_signal_emit (self, signals[FINISHED], 0);
}


/**
 * stm_transfer_start:
 * 
 * @self a #StmTransfer
 * 
 * Start a transfer. If transfer is already running this function
 * does nothing.
 * 
 */
void
stm_transfer_start (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	if (priv->state == STM_TRANSFER_STATE_STOPPED || priv->state == STM_TRANSFER_STATE_ERROR) {
		_stm_transfer_set_state (self, STM_TRANSFER_STATE_RUNNING);
		stm_transfer_open (self);
	}
}


/**
 * stm_transfer_stop:
 * 
 * @self a #StmTransfer
 * 
 * Stop a transfer. When transfer is not running, this function
 * does nothing. A #StmTransfer can be resumed later with
 * stm_transfer_start().
 * 
 */
void
stm_transfer_stop (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	if (priv->state == STM_TRANSFER_STATE_RUNNING) {
		StmTransferState state = (priv->completed == priv->length)
			? STM_TRANSFER_STATE_FINISHED : STM_TRANSFER_STATE_STOPPED; 
		_stm_transfer_set_state (self, state);
		stm_transfer_close (self);
	}
}


/**
 * _stm_transfer_to_xml:
 * 
 * Save transfer state in a XML node
 * 
 * Returns: A newly allocated gchar* representation od XML node. Free
 * with g_free when no longer used.
 */
gchar *
_stm_transfer_to_xml (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	return g_markup_printf_escaped ("    <transfer uri='%s'\n"
	                                "              file='%s'\n"
	                                "              downloaded='%llu'\n"
	                                "              total='%llu'\n"
	                                "              state='%d' />",
	                                priv->uri,
	                                priv->file,
	                                priv->completed,
	                                priv->length,
	                                priv->state);
}


/**
 * _stm_transfer_from_xml:
 * 
 * Restore transfer from XML node.
 * 
 * Returns: A new #StmTransfer, or NULL on failure.
 */
StmTransfer *
_stm_transfer_from_xml (const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values)
{
	const gchar *uri = NULL;
	const gchar *file = NULL;
	guint64 downloaded = 0;
	guint64 total = 0;
	int state = STM_TRANSFER_STATE_STOPPED;
	
	int i;
	for (i = 0; attribute_names[i]; i++) {
		if (strcmp (attribute_names[i], "uri") == 0) {
			uri = attribute_values[i];
		} else if (strcmp (attribute_names[i], "file") == 0) {
			file = attribute_values[i];
		} else if (strcmp (attribute_names[i], "downloaded") == 0) {
			downloaded =  g_ascii_strtoull(attribute_values[i], NULL, 10);
		} else if (strcmp (attribute_names[i], "total") == 0) {
			total = g_ascii_strtoull (attribute_values[i], NULL, 10);
		} else if (strcmp (attribute_names[i], "state") == 0) {
			state = atoi (attribute_values[i]);
		}
	}
	
	g_print ("Total=%llu\nCompleted=%llu\n", total, downloaded);
	
	if (uri && file) {
		StmTransfer *self = stm_transfer_new (uri, file);
		StmTransferPrivate *priv = self->priv;
		
		priv->completed = downloaded;
		priv->length = total;
		
		if (state > STM_TRANSFER_STATE_NONE && state <= STM_TRANSFER_STATE_ERROR) {
			if (state == STM_TRANSFER_STATE_RUNNING)
				stm_transfer_start (self);
			else
				_stm_transfer_set_state (self, state);
		}
		
		return self;
	} else {
		return NULL;
	}
}


/**
 * _open_file:
 * 
 * @file File name
 * 
 * Launch file with preferred application. Currently it calls
 * gnome-open. TODO: Use #GIO #GAppInfo if possible.
 */
static void _open_file (const gchar *file)
{
	gchar *arg = g_shell_quote (file);
	gchar *cmd = g_strdup_printf ("%s %s", "gnome-open", arg);
	
	g_print ("Executing \"%s\"\n", cmd);
	
	g_free (cmd);
	g_free (arg);
}


/**
 * stm_transfer_open_file:
 * 
 * @self A #StmTransfer
 * 
 * Launch a file using external application.
 * 
 * Note, this function does not check whether transfer has completed,
 * so it may be possible to launch incomplete file.
 */
void
stm_transfer_open_file (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	_open_file (priv->file);
}


/**
 * @self A #StmTransfer
 * 
 * Open directory where destination file is saved using external
 * apllication.
 *
 */
void
stm_transfer_open_directory (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	gchar *path = g_strdup (priv->file);
	gchar *slash = strrchr (path, '/');
	if (slash) {
		*(slash+1) = '\0';
		_open_file (path);
	} else {
		/* Open current directory */
		_open_file (".");
	}
	g_free (path);
}


/**
 * stm_transfer_get_state:
 * 
 * Get transfer state.
 * 
 * Returns: A #StmTransferState
 */
StmTransferState
stm_transfer_get_state                             (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	return priv->state;
}


/**
 * _stm_transfer_set_state:
 * 
 * Set transfer state.
 * 
 * Note that this function does not start/stop transfer. It just
 * sets appropriate flag and emits a signal.
 */
static void
_stm_transfer_set_state                            (StmTransfer *self,
                                                    StmTransferState state)
{
	StmTransferPrivate *priv = self->priv;

	priv->state = state;

	/* Fire progress to update views */
	g_signal_emit (self, signals[PROGRESS], 0);
}


/**
 * stm_transfer_get_uri:
 * 
 * @self: A #StmTransfer
 * 
 * Get URI for this transfer.
 * 
 * Returns: A URI. This value is internal and should not be modified
 * od freed.
 */
const gchar*
stm_transfer_get_uri                               (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	return priv->uri;
}


/**
 * stm_transfer_get_file:
 * 
 * @self: A #StmTransfer
 * 
 * Get full file path for this transfer.
 * 
 * Returns: A file path. This value is internal and should not be modified
 * od freed.
 */
const gchar*
stm_transfer_get_file                              (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	return priv->file;
}


/**
 * stm_transfer_get_file_name:
 * 
 * @self: A #StmTransfer
 * 
 * Get base file name for this transfer.
 * 
 * Returns: A file name. This value is internal and should not be modified
 * od freed.
 */
const gchar*
stm_transfer_get_file_name                         (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	return priv->file_name;
}



/**
 * stm_transfer_get_content_length:
 * 
 * @self: A #StmTransfer
 * 
 * Get total length of a transfer in bytes.
 * 
 * Returns: Length of transfer in bytes.
 */
guint64
stm_transfer_get_content_length                    (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

//	double ret;
//	if (curl_easy_getinfo (priv->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &ret) == CURLE_OK) {
//		return (guint64) ret;
//	} else {
//		return 0;
//	}
	return priv->length;
}


/**
 * stm_transfer_get_downloaded:
 * 
 * @self: A #StmTransfer
 * 
 * Get number of bytes that has been downloaded.
 * 
 * Returns: Number of bytes transferred.
 */
guint64
stm_transfer_get_downloaded                        (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

//	double ret;
//	if (curl_easy_getinfo (priv->curl, CURLINFO_SIZE_DOWNLOAD, &ret) == CURLE_OK) {
//		return (guint64) ret;
//	} else {
//		return 0;
//	}
	return priv->completed;
}


/**
 * stm_transfer_get_speed:
 * 
 * @self: A #StmTransfer
 * 
 * Get transfer speed measured in bytes per second (b/s).
 * 
 * Returns: Speed in b/s.
 */
guint64
stm_transfer_get_speed                             (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	if (priv->curl == NULL)
		return 0;
	
	double ret;
	if (curl_easy_getinfo (priv->curl, CURLINFO_SPEED_DOWNLOAD, &ret) == CURLE_OK) {
		return (guint64) ret;
	} else {
		return 0;
	}
}


/**
 * stm_transfer_get_total_time:
 * 
 * @self A #StmTransfer
 * 
 * Get number of seconds.
 */
guint64
stm_transfer_get_total_time                        (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;

	if (priv->curl == NULL)
		return 0;

	double ret;
	if (curl_easy_getinfo (priv->curl, CURLINFO_TOTAL_TIME, &ret) == CURLE_OK) {
		return (guint64) ret;
	} else {
		return 0;
	}
}


/**
 * stm_transfer_get_eta:
 * 
 * @self A #StmTransfer
 * 
 * Get estimated time left for this transger to complete (in seconds)
 * 
 * Returns: A number of seconds.
 */
guint64
stm_transfer_get_eta                               (StmTransfer *self)
{
//	StmTransferPrivate *priv = self->priv;

	guint64 size = stm_transfer_get_content_length (self);
	guint64 completed = stm_transfer_get_downloaded (self);
	guint64 speed = stm_transfer_get_speed (self);
	
	if (speed != 0)
		return (size - completed) / speed;
	else
		return 0;
}


/**
 * stm_transfer_get_md5:
 * 
 * @self A #StmTransfer
 * 
 * Return computed MD5 checksum for this transfer, or NULL. NULL will
 * be returned when transfer hasn't completed or STM was compiled
 * without OpenSSL support. This checksum is computed on-the-fly.
 * 
 * Return: A 32-character string representation of MD5 checksum, or
 * NULL if checksum is not available or STM was compiled without MD5
 * support.
 */
const gchar *
stm_transfer_get_md5 (StmTransfer *self)
{
#ifdef HAVE_CRYPTO
	StmTransferPrivate *priv = self->priv;
	return priv->md5;	
#else
	return NULL;
#endif
}


/**
 * stm_transfer_get_handle:
 * 
 * @self: A #StmTransfer
 * 
 * Return pointer to CURL easy handle.
 * 
 * Returns: A <structname>CURL<structname>
 */
void *
stm_transfer_get_handle (StmTransfer *self)
{
	StmTransferPrivate *priv = self->priv;
	
	return priv->curl;
}


/**
 * stm_transfer_write_data:
 * 
 * Callback used to write received data to disk. Arguments ar the 
 * same as to <function>fwrite</function>
 */
static size_t
stm_transfer_write_data (void *buffer, size_t size, size_t nmemb, void *userp)
{
	StmTransfer *self = STM_TRANSFER (userp);
	StmTransferPrivate *priv = self->priv;
	
/*
 	gsize bytes_written;
	GError *error = NULL;
	g_io_channel_write_chars (priv->io,
	                          buffer,
	                          size * nmemb,
	                          &bytes_written,
	                          &error);
	return bytes_written;
*/
	gsize bytes_written = fwrite (buffer, size, nmemb, priv->out);
	priv->completed += bytes_written;

#ifdef HAVE_CRYPTO
	MD5_Update (priv->md5_ctx, buffer, (unsigned long) size*nmemb);
#endif	
	
	return bytes_written;
}


/**
 * stm_transfer_progress_callback:
 * 
 * Callback used to monitor transfer progress
 * 
 */
static int
stm_transfer_progress_callback (void *clientp,
                                double dltotal,
                                double dlnow,
                                double ultotal,
                                double ulnow)
{
	StmTransfer *self = STM_TRANSFER (clientp);
	StmTransferPrivate *priv = self->priv;

	if (priv->length == 0)
		priv->length = (guint64) dltotal;
//	priv->completed = (guint64) dlnow;

	n_updates++;
	
	g_signal_emit (self, signals[PROGRESS], 0);
	return 0;
}


/**
 * stm_transfer_dispatch_message:
 * 
 * @self A #StmTransfer
 * @msg A CURL Message to be dispatched
 * 
 * Dispatch a CURL message.
 * 
 */
static void
stm_transfer_dispatch_message (StmTransfer *self, CURLMsg *msg)
{
	StmTransferPrivate *priv = self->priv;
	g_print ("[%s]: got msg %d\n", priv->file, msg->msg);
	switch (msg->msg) {
		case CURLMSG_DONE: { // transfer finished
			stm_transfer_finish (self, msg->data.result);
		} break;
		
		default:
			g_printerr ("Unrecognized message %d\n", msg->msg);
	}
}


/**
 * stm_transfer_curl_callback:
 * 
 * Callback to CURL events.
 * 
 * Find relevant transfer and deliver message to that transfer.
 * 
 * @see_also stm_transfer_dispatch_message()
 */
static void
stm_transfer_curl_callback (void *data)
{
	int msgs;
	CURLMsg *msg;
	
	while ((msg = curl_multi_info_read(glibcurl_handle(), &msgs)) != NULL) {
		StmTransfer *transfer = (StmTransfer *) g_hash_table_lookup (all_transfers, msg->easy_handle);
		if (transfer == NULL) {
			g_printerr ("Unregistered handle %p!\n", msg->easy_handle);
		} else {
			stm_transfer_dispatch_message (transfer, msg);
		}
	}
}

/* GObject implementation */

static void
stm_transfer_init (StmTransfer *self)
{
	self->priv = STM_TRANSFER_GET_PRIVATE (self);
	StmTransferPrivate *priv = self->priv;

	priv->disposed = FALSE;
	
	priv->file = NULL;
	priv->uri = NULL;
	
	priv->out = NULL;
	priv->curl = NULL;
	priv->state = STM_TRANSFER_STATE_STOPPED;
	priv->last_time = 0;
	priv->speed = 0.0f;
	
	priv->error_buffer = g_new (gchar, CURL_ERROR_SIZE);
	
#ifdef HAVE_CRYPTO
	priv->md5_ctx = g_new (MD5_CTX, 1);
	MD5_Init (priv->md5_ctx);
	priv->md5 = NULL;
#endif
	
//	g_print ("stm_transfer_init ()\n");
}


static void
stm_transfer_dispose (GObject *object)
{
	StmTransfer *self = (StmTransfer*) object;
	StmTransferPrivate *priv = self->priv;

	g_print ("stm_transfer_dispose ()\n");
	/* Make sure dispose is called only once */
	if (priv->disposed) {
		return;
	}
	priv->disposed = TRUE;
	
	if (priv->out) {
		stm_transfer_close (self);
	}
	
	g_free (priv->uri);
	g_free (priv->file);
	g_free (priv->error_buffer);
	g_free (priv->error_msg);

#ifdef HAVE_CRYPTO
	g_free (priv->md5);
	g_free (priv->md5_ctx);
#endif

	/* Unregister global message handler */
	g_hash_table_remove (all_transfers, priv->curl);

	/* Clean up */
	curl_easy_cleanup (priv->curl);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (stm_transfer_parent_class)->dispose (object);
}


static void
stm_transfer_finalize (GObject *object)
{
	G_OBJECT_CLASS (stm_transfer_parent_class)->finalize (object);
}

	
static void
stm_transfer_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
	StmTransfer* self = STM_TRANSFER (object);
	StmTransferPrivate* priv = self->priv;

	switch (property_id) {
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
			
		case PROP_FILE:
			g_value_set_string (value, priv->file);
			break;
			
		case PROP_COMPLETED:
			g_value_set_uint64 (value, priv->completed);
			break;
			
		case PROP_CONTENT_LENGTH:
			g_value_set_uint64 (value, priv->length);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_transfer_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	StmTransfer* self = STM_TRANSFER (object);
	StmTransferPrivate* priv = self->priv;

	switch (property_id) {
		case PROP_URI:
			g_free (priv->uri);
			priv->uri = g_value_dup_string (value);
			STM_DEBUG_PROP("url", priv->uri);
			break;
			
		case PROP_FILE:
			g_free (priv->file);
			priv->file = g_value_dup_string (value);
			STM_DEBUG_PROP("file", priv->file);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
stm_transfer_class_init (StmTransferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = stm_transfer_get_property;
	gobject_class->set_property = stm_transfer_set_property;
	gobject_class->dispose = stm_transfer_dispose;
	gobject_class->finalize = stm_transfer_finalize;

	g_type_class_add_private (klass, sizeof (StmTransferPrivate));
	
	/* Signals */
	signals[PROGRESS] = g_signal_new ("progress",
	                                  G_TYPE_FROM_CLASS (klass),
	                                  G_SIGNAL_RUN_LAST,
	                                  G_STRUCT_OFFSET (StmTransferClass, progress),
	                                  NULL, NULL,
	                                  g_cclosure_marshal_VOID__VOID,
	                                  G_TYPE_NONE, 0);
	                                  
	signals[STARTED] = g_signal_new ("started",
	                                  G_TYPE_FROM_CLASS (klass),
	                                  G_SIGNAL_RUN_LAST,
	                                  G_STRUCT_OFFSET (StmTransferClass, started),
	                                  NULL, NULL,
	                                  g_cclosure_marshal_VOID__VOID,
	                                  G_TYPE_NONE, 0);
	                                  
	signals[FINISHED] = g_signal_new ("finished",
	                                  G_TYPE_FROM_CLASS (klass),
	                                  G_SIGNAL_RUN_LAST,
	                                  G_STRUCT_OFFSET (StmTransferClass, finished),
	                                  NULL, NULL,
	                                  g_cclosure_marshal_VOID__VOID,
	                                  G_TYPE_NONE, 0);
	                                  
	                                  
	                                  
	
	/* Properties */
	
	g_object_class_install_property (gobject_class,
	                                 PROP_URI,
	                                 g_param_spec_string (
	                                 "uri",
	                                 "URI to download",
	                                 "Get URI location",
	                                 NULL,
	                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
	                                 PROP_FILE,
	                                 g_param_spec_string (
	                                 "file",
	                                 "Download location",
	                                 "Where file will be downloaded to",
	                                 NULL,
	                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
	                                 
	g_object_class_install_property (gobject_class,
	                                 PROP_CONTENT_LENGTH,
	                                 g_param_spec_uint64 (
	                                 "content-length",
	                                 "Content length",
	                                 "Get content length",
	                                 0,				/* min */
	                                 G_MAXUINT64,	/* max */
	                                 0,				/* default */
	                                 G_PARAM_READABLE));
	                                 
	g_object_class_install_property (gobject_class,
	                                 PROP_COMPLETED,
	                                 g_param_spec_uint64 (
	                                 "completed",
	                                 "Number of downloaded bytes",
	                                 "Get number of completed bytes",
	                                 0,				/* min */
	                                 G_MAXUINT64,	/* max */
	                                 0,				/* default */
	                                 G_PARAM_READABLE));
	                                 
	/* Global message dispatcher, ugly */
	all_transfers = g_hash_table_new (g_direct_hash, g_direct_equal);
	glibcurl_set_callback (stm_transfer_curl_callback, NULL);
}

