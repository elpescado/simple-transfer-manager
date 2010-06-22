#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "stm-manager.h"
#include "stm-transfer.h"
#include "stm-transfer-window.h"
#include "stm-panel.h"
#include "stm-main-window.h"

#ifdef STM_POSIX
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
#  define FIFO "stm.fifo"
#  define MODE 0666
#endif

GtkWidget *main_window;


void progress (StmTransfer *transfer)
{
	g_print ("Transfer! %lld of %lld, %lld b/S\n",
	         stm_transfer_get_downloaded (transfer),
	         stm_transfer_get_content_length (transfer),
	         stm_transfer_get_speed (transfer)
	         );
}

#ifdef STM_POSIX


/**
 * fifo_callback:
 *
 * Function called when a IPC message is received
 */
static gboolean
fifo_callback (GIOChannel *channel,
               GIOCondition condition,
               StmManager *manager)
{
	gchar buffer[2048];
	gsize bytes_read;

	g_io_channel_read_chars (channel, buffer, 2048, &bytes_read, NULL);
	buffer[bytes_read-1] = '\0';

	const gchar *cmd = buffer;
	gchar *arg = strchr (buffer, ' ');
	if (arg) {
		*arg = '\0'; arg++;
		while (*arg && isspace (*arg))
			arg++;

	}

	if (strcmp (cmd, "get") == 0) {
		gchar *cwd = g_get_current_dir ();
		StmTransfer *xfer = stm_transfer_new (arg, cwd);
		stm_manager_add_transfer (manager, xfer);
		stm_transfer_start (xfer);
		g_object_unref (xfer);
		g_free (cwd);
	} else if (strcmp (cmd, "show") == 0) {
		gtk_window_present (GTK_WINDOW (main_window));
	}
	return TRUE;
}


/**
 * fifo_setup:
 *
 * Set up a pipe so that this process can handle
 * messages from further stm invocations
 */
gboolean fifo_setup (StmManager *m)
{
	gchar *fifo_file = stm_find_user_file (FIFO);
	if (mknod (fifo_file, S_IFIFO | MODE, 0) < 0) {
		g_printerr ("Cannot create FIFO '%s' for IPC\n", fifo_file);
	}

	int fd = open (fifo_file, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		g_printerr ("Cannot open FIFO '%s' for IPC\n", fifo_file);
		return FALSE;
	}

	GIOChannel *channel = g_io_channel_unix_new (fd);
	if (channel == NULL) {
		g_printerr ("Cannot open FIFO '%s' for IPC\n", fifo_file);
		return FALSE;
	}

	g_io_add_watch (channel, G_IO_IN, (GIOFunc) fifo_callback, m);
	g_free (fifo_file);

	return TRUE;
}
#endif

int main (int argc, char *argv[])
{
	gchar *url = argv[1];
	gchar *state_file = stm_find_user_file ("state.xml");
	gchar *fifo_file = stm_find_user_file (FIFO);

#ifdef STM_POSIX
	if (g_file_test (fifo_file, G_FILE_TEST_EXISTS)) {
		gchar *msg;
		if (url) {
			msg = g_strdup_printf ("get %s\n", url);
		} else {
			msg = g_strdup ("show\n");
		}

//		FILE *f = fopen (FIFO, "w");
//		fwrite (msg, sizeof (gchar), strlen (msg), f);
//		fclose (f);
		
		GIOChannel *channel = g_io_channel_new_file (fifo_file, "w", NULL);
		gsize bytes_written;
		g_io_channel_write_chars (channel, msg, strlen (msg), &bytes_written, NULL);
		g_io_channel_shutdown (channel, TRUE, NULL);
		g_io_channel_unref (channel);
		

		g_free (msg);
		return 0;
	}
#endif
	gtk_init (&argc, &argv);

	g_print ("Using state file: %s\n", state_file);
	
	StmManager *m = stm_manager_new ();
	stm_manager_load_state (m, state_file);
	stm_manager_set_state_file (m, state_file);

#ifdef STM_POSIX	
	fifo_setup (m);
#endif
	
	main_window = stm_main_window_new (m);
	gtk_widget_show (main_window);
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (gtk_main_quit), NULL);

	/*
	if (argc > 1) {
		g_print ("Adding transfers\n");
	//	StmTransfer *xfer1 = stm_transfer_new ("http://localhost/~przemek/test.txt", "test2.txt");
		StmTransfer *xfer2 = stm_transfer_new ("http://localhost/~przemek/ubuntu/ubuntu-8.04-alternate-i386.iso", "/home/przemek/ubuntu.iso");
//		StmTransfer *xfer2 = stm_transfer_new ("http://localhost/~przemek/ubuntu/minibuntu-7.10-i386.iso", "/home/przemek/ubuntu.iso");
//		StmTransfer *xfer2 = stm_transfer_new ("http://sunsite.icm.edu.pl/pub/Linux/opensuse/distribution/10.3/iso/cd/openSUSE-10.3-GM-GNOME-Live-i386.iso", "openSUSE-10.3-GM-GNOME-Live-i386.iso");
	//	stm_manager_add_transfer (m, xfer1);
		stm_manager_add_transfer (m, xfer2);
//		stm_transfer_start (xfer2);
		g_object_unref (xfer2);
	}
	*/

	GtkWidget *tw;
/*	
	tw = stm_transfer_window_new ();
	stm_transfer_window_set_transfer (STM_TRANSFER_WINDOW (tw), xfer1);
	gtk_widget_show (tw);

	tw = stm_transfer_window_new ();
	stm_transfer_window_set_transfer (STM_TRANSFER_WINDOW (tw), xfer2);
	gtk_widget_show (tw);
*/	

	/* Some benchmarks */
	extern gint n_updates;
	time_t t0 = time (NULL);
	gtk_main ();
	time_t t1 = time (NULL);
	g_print ("--> %d updates in %d seconds\n", n_updates, t1 - t0);
	
	stm_manager_save_state (m, state_file);
	g_object_unref (m);
	g_free (state_file);

#ifdef STM_POSIX	
	unlink (fifo_file);
	g_free (fifo_file);
#endif

	return 0;
}
