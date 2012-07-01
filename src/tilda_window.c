/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <cilda-config.h>

#include <debug.h>
#include <tilda.h>
#include <configsys.h>
#include <tilda_window.h>
#include <tilda_terminal.h>
#include <key_grabber.h>
#include <translation.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vte/vte.h>

static void
tilda_window_setup_alpha_mode (tilda_window *tw)
{
    GdkScreen *screen;
    GdkVisual *visual;

    screen = gtk_widget_get_screen (GTK_WIDGET (tw->window));
    visual = gdk_screen_get_rgba_visual (screen);
    if (visual != NULL )
    {
        /* Set RGBA colormap if possible so VTE can use real alpha
         * channels for transparency. */

        gtk_widget_set_visual(GTK_WIDGET (tw->window), visual);
        tw->have_argb_visual = TRUE;
    }
    else
    {
        tw->have_argb_visual = FALSE;
    }
}

static gint full_screen_window (tilda_window *tw)
{
	static gint s_is_full = FALSE;
	static gint sh;
	static gint sw;
	static gint sx;
	static gint sy;

	if (s_is_full) {
		s_is_full = FALSE;
		gtk_window_unmaximize(GTK_WINDOW(tw->window));
        //screen = gdk_screen_get_default();
        //awin = gdk_screen_get_active_window(screen);
        //gtk_window_move (GTK_WINDOW(tw->window), rt.x + config_getint ("x_pos"), rt.y + config_getint ("y_pos"));

		gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
		gtk_window_resize (GTK_WINDOW(tw->window), sw, sh);
		gtk_window_move (GTK_WINDOW(tw->window), sx, sy);
	} else {
		s_is_full = TRUE;
		gtk_window_get_size(GTK_WINDOW(tw->window), &sw, &sh);
		gtk_window_get_position(GTK_WINDOW(tw->window), &sx, &sy);
		gtk_window_maximize(GTK_WINDOW(tw->window));
	}
	return TRUE;
}

static gint zoom_up_window (tilda_window *tw)
{
	gint w, h, sw, sh;
	GdkScreen* screen;
	screen = gdk_screen_get_default();
	sw = gdk_screen_get_width(screen);
	sh = gdk_screen_get_height(screen);
	gtk_window_get_size(GTK_WINDOW(tw->window), &w, &h);
	h += 10;
	if ( h > sh)
		h = sh;
	gtk_window_resize (GTK_WINDOW(tw->window), w, h);
	return TRUE;
}

static gint zoom_down_window (tilda_window *tw)
{
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(tw->window), &w, &h);
	h -= 10;
	if ( h < 20)
		h = 20;
	gtk_window_resize (GTK_WINDOW(tw->window), w, h);
	return TRUE;
}

static gint ccopy (tilda_window *tw)
{
    DEBUG_FUNCTION ("ccopy");
    DEBUG_ASSERT (tw != NULL);

    vte_terminal_copy_clipboard (tw->term->vte_term);

    /* Stop the event's propagation */
    return TRUE;
}

static gint cpaste (tilda_window *tw)
{
    DEBUG_FUNCTION ("cpaste");
    DEBUG_ASSERT (tw != NULL);
    vte_terminal_paste_clipboard (tw->term->vte_term);

    /* Stop the event's propagation */
    return TRUE;
}

static gint tilda_window_setup_keyboard_accelerators (tilda_window *tw)
{
    GtkAccelGroup *accel_group;
    GClosure *temp;

    /* Create Accel Group to add key codes for quit, next, prev and new tabs */
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), accel_group);

    /* Exit on <Ctrl><Shift>q */
    temp = g_cclosure_new_swap (G_CALLBACK(gtk_main_quit), tw, NULL);
    gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE, temp);

    /* zoom up window */
    temp = g_cclosure_new_swap (G_CALLBACK(zoom_down_window), tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_KEY_Up, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, temp);

    /* zoom down window */
    temp = g_cclosure_new_swap (G_CALLBACK(zoom_up_window), tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_KEY_Down, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, temp);

    /* zoom down window */
    temp = g_cclosure_new_swap (G_CALLBACK(full_screen_window), tw, NULL);
    gtk_accel_group_connect (accel_group, GDK_KEY_F11, 0, GTK_ACCEL_VISIBLE, temp);

    return 0;
}

static gint tilda_window_set_icon (tilda_window *tw, gchar *filename)
{
    GdkPixbuf *window_icon = gdk_pixbuf_new_from_file (filename, NULL);

    if (window_icon == NULL)
    {
        TILDA_PERROR ();
        DEBUG_ERROR ("Cannot open window icon");
        g_printerr (_("Unable to set tilda's icon: %s\n"), filename);
        return 1;
    }

    gtk_window_set_icon (GTK_WINDOW(tw->window), window_icon);
    g_object_unref (window_icon);

    return 0;
}

/**
 * tilda_window_init ()
 *
 * Create a new tilda_window * and return it. It will also initialize and set up
 * as much of the window as possible using the values in the configuation system.
 *
 * @param instance the instance number of this tilda_window
 *
 * Success: return a non-NULL tilda_window *
 * Failure: return NULL
 *
 * Notes: The configuration system must be up and running before calling this function.
 */
tilda_window *tilda_window_init (const gchar *config_file)
{
    DEBUG_FUNCTION ("tilda_window_init");
    DEBUG_ASSERT (instance >= 0);

    tilda_window *tw;

    tw = g_malloc (sizeof(tilda_window));

    if (tw == NULL)
        return NULL;

    /* Get the user's home directory */
    tw->home_dir = g_strdup (g_get_home_dir ());

    /* Set the config file */
    tw->config_file = g_strdup (config_file);

    /* Create the main window */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* Set up all window properties */
    if (config_getbool ("pinned"))
        gtk_window_stick (GTK_WINDOW(tw->window));

    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tw->window), config_getbool ("notaskbar"));
    gtk_window_set_keep_above (GTK_WINDOW(tw->window), config_getbool ("above"));
    gtk_window_set_decorated (GTK_WINDOW(tw->window), FALSE);
    gtk_widget_set_size_request (GTK_WIDGET(tw->window), 0, 0);
    tilda_window_set_icon (tw, g_build_filename (DATADIR, "pixmaps", "cilda.png", NULL));
    tilda_window_setup_alpha_mode (tw);

    /* Add keyboard accelerators */
    tilda_window_setup_keyboard_accelerators (tw);

    GtkWidget *label;
    gint index;

    tw->term = tilda_term_init (tw);

    gtk_widget_show_all(tw->term);

    if (tw->term == NULL)
    {
        TILDA_PERROR ();
        g_printerr (_("Out of memory, cannot create tab\n"));
        return FALSE;
    }

    gtk_container_add(tw->window,tw->term->hbox);

    /* The new terminal should grab the focus automatically */
    gtk_widget_grab_focus (tw->term->vte_term);

    /* Connect signal handlers */
    g_signal_connect (G_OBJECT(tw->window), "delete_event", (gtk_main_quit), tw->window);

    /* Position the window */
    tw->current_state = UP;
    gtk_window_move (GTK_WINDOW(tw->window), config_getint ("x_pos"), config_getint ("y_pos"));
    gtk_window_set_default_size (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    gtk_window_resize (GTK_WINDOW(tw->window), config_getint ("max_width"), config_getint ("max_height"));
    /* Create GDK resources now, to prevent crashes later on */
    gtk_widget_realize (tw->window);

    generate_animation_positions (tw);

    return tw;
}

gint tilda_window_free (tilda_window *tw)
{
    g_free (tw->config_file);
    g_free (tw->home_dir);

    g_free (tw);

    return 0;
}
