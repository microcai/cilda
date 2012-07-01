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

#include <cilda-config.h>

#include <debug.h>
#include <configsys.h>
#include <tilda_window.h>
#include <key_grabber.h> /* for pull */
#include <translation.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <pwd.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xrandr.h>

#include <vte/vte.h>
#include <glib/gstdio.h>

#include "tilda.h"
#include "wizard.h"
#include "tomboykeybinder.h"

/**
 * Parse all of the Command-Line Options given to tilda.
 * This can modify argv and argc, and will set values in the config.
 *
 * @param argc argc from main
 * @param argv argv from main
 * @return TRUE if we should show the configuration wizard, FALSE otherwise
 */
static gboolean parse_cli (int argc, char *argv[])
{
    DEBUG_FUNCTION ("parse_cli");
    DEBUG_ASSERT (argc != NULL);
    DEBUG_ASSERT (argv != NULL);

    /* Set default values */
    gchar *background_color = config_getstr ("background_color");
    gchar *command = config_getstr ("command");
    gchar *font = config_getstr ("font");
    gchar *image = config_getstr ("image");
    gchar *working_dir = config_getstr ("working_dir");

    gint lines = config_getint ("lines");
    gint transparency = config_getint ("transparency");
    gint x_pos = config_getint ("x_pos");
    gint y_pos = config_getint ("y_pos");

    gboolean antialias = config_getbool ("antialias");
    gboolean scrollbar = config_getbool ("scrollbar");
    gboolean show_config = FALSE;
    gboolean version = FALSE;
    gboolean hidden = config_getbool ("hidden");

    /* All of the various command-line options */
    GOptionEntry cl_opts[] = {
        { "antialias",          'a', 0, G_OPTION_ARG_NONE,      &antialias,         _("Use Antialiased Fonts"), NULL },
        { "background-color",   'b', 0, G_OPTION_ARG_STRING,    &background_color,  _("Set the background color"), NULL },
        { "command",            'c', 0, G_OPTION_ARG_STRING,    &command,           _("Run a command at startup"), NULL },
        { "hidden",             'h', 0, G_OPTION_ARG_NONE,      &hidden,            _("Start Tilda hidden"), NULL },
        { "font",               'f', 0, G_OPTION_ARG_STRING,    &font,              _("Set the font to the following string"), NULL },
        { "lines",              'l', 0, G_OPTION_ARG_INT,       &lines,             _("Scrollback Lines"), NULL },
        { "scrollbar",          's', 0, G_OPTION_ARG_NONE,      &scrollbar,         _("Use Scrollbar"), NULL },
        { "transparency",       't', 0, G_OPTION_ARG_INT,       &transparency,      _("Opaqueness: 0-100%"), NULL },
        { "version",            'v', 0, G_OPTION_ARG_NONE,      &version,           _("Print the version, then exit"), NULL },
        { "working-dir",        'w', 0, G_OPTION_ARG_STRING,    &working_dir,       _("Set Initial Working Directory"), NULL },
        { "x-pos",              'x', 0, G_OPTION_ARG_INT,       &x_pos,             _("X Position"), NULL },
        { "y-pos",              'y', 0, G_OPTION_ARG_INT,       &y_pos,             _("Y Position"), NULL },
        { "image",              'B', 0, G_OPTION_ARG_STRING,    &image,             _("Set Background Image"), NULL },
        { "config",             'C', 0, G_OPTION_ARG_NONE,      &show_config,       _("Show Configuration Wizard"), NULL },
        { NULL }
    };


    /* Set up the command-line parser */
    GError *error = NULL;
    GOptionContext *context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, cl_opts, NULL);
	GIOChannel* remote_control_sock;
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);

    /* Check for unknown options, and give a nice message if there are some */
    if (error)
    {
        const char *msg = _("Error parsing command-line options. Try \"tilda --help\"\nto see all possible options.\n\nError message: %s\n");
        g_printerr (msg, error->message);

        exit (EXIT_FAILURE);
    }

    /* If we need to show the version, show it then exit normally */
    if (version)
    {
        g_print ("%s\n\n", TILDA_VERSION);

        g_print ("Copyright (c) 2005-2008 Tristan Sloughter (sloutri@iit.edu)\n");
        g_print ("Copyright (c) 2005-2008 Ira W. Snyder (tilda@irasnyder.com)\n\n");

        g_print ("General Information: http://launchpad.net/tilda\n");
        g_print ("Bug Reports: http://bugs.launchpad.net/tilda\n\n");

        g_print ("This program comes with ABSOLUTELY NO WARRANTY.\n");
        g_print ("This is free software, and you are welcome to redistribute it\n");
        g_print ("under certain conditions. See the file COPYING for details.\n");

        exit (EXIT_SUCCESS);
    }

    /* Now set the options in the config, if they changed */
    if (background_color != config_getstr ("background_color"))
        config_setstr ("background_color", background_color);
    if (command != config_getstr ("command"))
    {
        config_setbool ("run_command", TRUE);
        config_setstr ("command", command);
    }
    if (font != config_getstr ("font"))
        config_setstr ("font", font);
    if (image != config_getstr ("image"))
        config_setstr ("image", image);
    if (working_dir != config_getstr ("working_dir"))
        config_setstr ("working_dir", working_dir);

    if (lines != config_getint ("lines"))
        config_setint ("lines", lines);
    if (transparency != config_getint ("transparency"))
    {
        config_setbool ("enable_transparency", transparency);
        config_setint ("transparency", transparency);
    }
    if (x_pos != config_getint ("x_pos"))
        config_setint ("x_pos", x_pos);
    if (y_pos != config_getint ("y_pos"))
        config_setint ("y_pos", y_pos);

    if (antialias != config_getbool ("antialias"))
        config_setbool ("antialias", antialias);
    if (hidden != config_getbool ("hidden"))
        config_setbool ("hidden", hidden);
    if (scrollbar != config_getbool ("scrollbar"))
        config_setbool ("scrollbar", scrollbar);

    /* TRUE if we should show the config wizard, FALSE otherwize */
    return show_config;
}

/*
 * Finds the coordinate that will center the tilda window in the screen.
 *
 * If you want to center the tilda window on the top or bottom of the screen,
 * pass the screen width into screen_dimension and the tilda window's width
 * into the tilda_dimension variable. The result will be the x coordinate that
 * should be used in order to have the tilda window centered on the screen.
 *
 * Centering based on y coordinate is similar, just use the screen height and
 * tilda window height.
 */
int find_centering_coordinate (const int screen_dimension, const int tilda_dimension)
{
    DEBUG_FUNCTION ("find_centering_coordinate");

    const float screen_center = screen_dimension / 2.0;
    const float tilda_center  = tilda_dimension  / 2.0;

    return screen_center - tilda_center;
}

/**
 * Get a pointer to the config file name for this instance.
 *
 * NOTE: you MUST call free() on the returned value!!!
 *
 * @param tw the tilda_window structure corresponding to this instance
 * @return a pointer to a string representation of the config file's name
 */
static gchar *get_config_file_name ()
{
    DEBUG_FUNCTION ("get_config_file_name");
    DEBUG_ASSERT (home_directory != NULL);
    DEBUG_ASSERT (instance >= 0);

    gchar *config_file = g_build_filename (g_get_home_dir(), ".config", "tilda", NULL);

    return config_file;
}

int main (int argc, char *argv[])
{
    DEBUG_FUNCTION ("main");

    tilda_window *tw = NULL;

    gboolean need_wizard = FALSE;
    gchar *config_file;


    config_file = get_config_file_name ();

#if ENABLE_NLS
    /* Gettext Initialization */
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif


#ifdef DEBUG
    /* Have to do this early. */
    if (getenv ("VTE_PROFILE_MEMORY"))
        if (atol (getenv ("VTE_PROFILE_MEMORY")) != 0)
            g_mem_set_vtable (glib_mem_profiler_table);
#endif
    /* Start up the configuration system */
    config_init (config_file);

    /* Parse the command line */
    need_wizard = parse_cli (argc, argv);

    if (!g_thread_supported ())
    	g_error("Need glib with thread support!");

    /* Initialize GTK and libglade */
    gtk_init (&argc, &argv);

    /* create new tilda_window */
    tw = tilda_window_init (config_file);

    /* Check the allocations above */
    if (tw == NULL)
        return -EFAULT;

    /* Initialize and set up the keybinding to toggle tilda's visibility. */
    tomboy_keybinder_init ();

    /* If the config file doesn't exist open up the wizard */
    if (access (tw->config_file, R_OK) == -1)
    {
        config_setstr ("key", "F1");
        need_wizard = TRUE;
    }

    /* Show the wizard if we need to.
     *
     * Note that the key will be bound upon exiting the wizard */
    if (need_wizard)
        wizard (tw);
    else
    {
        gint ret = tilda_keygrabber_bind (config_getstr ("key"), tw);

        if (!ret)
        {
            /* The key was unbindable, so we need to show the wizard */
            show_invalid_keybinding_dialog (NULL);
            wizard (tw);
        }
    }

    if (config_getbool ("hidden"))
    {
        /* It does not cause graphical glitches to make tilda hidden on start this way.
         * It does make tilda appear much faster on it's first appearance, so I'm leaving
         * it this way, because it has a good benefit, and no apparent drawbacks. */
        gtk_widget_show_all(GTK_WIDGET(tw->window));
        gtk_widget_hide (GTK_WIDGET(tw->window));
    }
    else
    {
        pull (tw, PULL_DOWN);
    }

    /* Whew! We're finally all set up and ready to run GTK ... */
    gtk_main();
    return 0;
}

