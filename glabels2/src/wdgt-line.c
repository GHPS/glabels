/*
 *  (GLABELS) Label and Business Card Creation program for GNOME
 *
 *  wdgt_line.c:  line properties widget module
 *
 *  Copyright (C) 2001-2002  Jim Evins <evins@snaught.com>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <config.h>

#include "mygal/widget-color-combo.h"
#include "prefs.h"
#include "wdgt-line.h"
#include "marshal.h"
#include "color.h"

#include "debug.h"

/*===========================================*/
/* Private types                             */
/*===========================================*/

enum {
	CHANGED,
	LAST_SIGNAL
};

typedef void (*glWdgtLineSignal) (GObject * object, gpointer data);

/*===========================================*/
/* Private globals                           */
/*===========================================*/

static GObjectClass *parent_class;

static gint wdgt_line_signals[LAST_SIGNAL] = { 0 };

/*===========================================*/
/* Local function prototypes                 */
/*===========================================*/

static void gl_wdgt_line_class_init    (glWdgtLineClass *class);
static void gl_wdgt_line_instance_init (glWdgtLine      *line);
static void gl_wdgt_line_finalize      (GObject         *object);
static void gl_wdgt_line_construct     (glWdgtLine      *line);

static void changed_cb                 (glWdgtLine      *line);


/****************************************************************************/
/* Boilerplate Object stuff.                                                */
/****************************************************************************/
guint
gl_wdgt_line_get_type (void)
{
	static guint wdgt_line_type = 0;

	if (!wdgt_line_type) {
		GTypeInfo wdgt_line_info = {
			sizeof (glWdgtLineClass),
			NULL,
			NULL,
			(GClassInitFunc) gl_wdgt_line_class_init,
			NULL,
			NULL,
			sizeof (glWdgtLine),
			0,
			(GInstanceInitFunc) gl_wdgt_line_instance_init,
		};

		wdgt_line_type =
		    g_type_register_static (gl_hig_vbox_get_type (),
					    "glWdgtLine",
					    &wdgt_line_info, 0);
	}

	return wdgt_line_type;
}

static void
gl_wdgt_line_class_init (glWdgtLineClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) class;

	parent_class = g_type_class_peek_parent (class);

	object_class->finalize = gl_wdgt_line_finalize;

	wdgt_line_signals[CHANGED] =
	    g_signal_new ("changed",
			  G_OBJECT_CLASS_TYPE(object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (glWdgtLineClass, changed),
			  NULL, NULL,
			  gl_marshal_VOID__VOID,
			  G_TYPE_NONE, 0);

}

static void
gl_wdgt_line_instance_init (glWdgtLine *line)
{
	line->width_spin = NULL;
	line->color_picker = NULL;
	line->units_label = NULL;
}

static void
gl_wdgt_line_finalize (GObject *object)
{
	glWdgtLine *line;
	glWdgtLineClass *class;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GL_IS_WDGT_LINE (object));

	line = GL_WDGT_LINE (object);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/****************************************************************************/
/* New widget.                                                              */
/****************************************************************************/
GtkWidget *
gl_wdgt_line_new (void)
{
	glWdgtLine *line;

	line = g_object_new (gl_wdgt_line_get_type (), NULL);

	gl_wdgt_line_construct (line);

	return GTK_WIDGET (line);
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Construct composite widget.                                    */
/*--------------------------------------------------------------------------*/
static void
gl_wdgt_line_construct (glWdgtLine *line)
{
	GtkWidget  *wvbox, *wframe, *whbox;
	GtkObject  *adjust;
	ColorGroup *cg;
	GdkColor   *gdk_color;

	wvbox = GTK_WIDGET (line);

	/* ---- Line width line ---- */
	whbox = gl_hig_hbox_new ();
	gl_hig_vbox_add_widget (GL_HIG_VBOX(wvbox), whbox);

	/* Line Width Label */
	line->width_label = gtk_label_new (_("Width:"));
	gtk_misc_set_alignment (GTK_MISC (line->width_label), 0, 0.5);
	gl_hig_hbox_add_widget (GL_HIG_HBOX(whbox), line->width_label);

	/* Line Width widget */
	adjust = gtk_adjustment_new (1.0, 0.25, 4.0, 0.25, 1.0, 1.0);
	line->width_spin =
	    gtk_spin_button_new (GTK_ADJUSTMENT (adjust), 0.25, 2);
	g_signal_connect_swapped (G_OBJECT (line->width_spin), "changed",
				  G_CALLBACK (changed_cb),
				  G_OBJECT (line));
	gl_hig_hbox_add_widget (GL_HIG_HBOX(whbox), line->width_spin);

	/* Line Width units */
	line->units_label = gtk_label_new (_("points"));
	gtk_misc_set_alignment (GTK_MISC (line->units_label), 0, 0.5);
	gl_hig_hbox_add_widget (GL_HIG_HBOX(whbox), line->units_label);

	/* ---- Line color line ---- */
	whbox = gl_hig_hbox_new ();
	gl_hig_vbox_add_widget (GL_HIG_VBOX(wvbox), whbox);

	/* Line Color Label */
	line->color_label = gtk_label_new (_("Color:"));
	gtk_misc_set_alignment (GTK_MISC (line->color_label), 0, 0.5);
	gl_hig_hbox_add_widget (GL_HIG_HBOX(whbox), line->color_label);

	/* Line Color picker widget */
        cg = color_group_fetch ("line_color_group", NULL);
        gdk_color = gl_color_to_gdk_color (gl_prefs->default_line_color);
        line->color_picker = color_combo_new (NULL, _("No line"), gdk_color, cg);
	color_combo_box_set_preview_relief (COLOR_COMBO(line->color_picker),
					    GTK_RELIEF_NORMAL);
        g_free (gdk_color);
	g_signal_connect_swapped (G_OBJECT (line->color_picker), "color_changed",
				  G_CALLBACK (changed_cb),
				  G_OBJECT (line));
	gl_hig_hbox_add_widget (GL_HIG_HBOX(whbox), line->color_picker);

}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Callback for when any control in the widget has changed.       */
/*--------------------------------------------------------------------------*/
static void
changed_cb (glWdgtLine *line)
{
	/* Emit our "changed" signal */
	g_signal_emit (G_OBJECT (line), wdgt_line_signals[CHANGED], 0);
}

/****************************************************************************/
/* query values from controls.                                              */
/****************************************************************************/
void
gl_wdgt_line_get_params (glWdgtLine *line,
			 gdouble    *width,
			 guint      *color)
{
	GdkColor *gdk_color;
	gboolean  is_default;

	*width =
	    gtk_spin_button_get_value (GTK_SPIN_BUTTON(line->width_spin));

	gdk_color = color_combo_get_color (COLOR_COMBO(line->color_picker),
					   &is_default);

	if (is_default) {
		*color = GL_COLOR_NONE;
	} else {
		*color = gl_color_from_gdk_color (gdk_color);
	}
}

/****************************************************************************/
/* fill in values and ranges for controls.                                  */
/****************************************************************************/
void
gl_wdgt_line_set_params (glWdgtLine *line,
			 gdouble     width,
			 guint       color)
{
	GdkColor *gdk_color;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (line->width_spin), width);

	gdk_color = gl_color_to_gdk_color (color);
	color_combo_set_color (COLOR_COMBO(line->color_picker), gdk_color);
	g_free (gdk_color);
			       
}

/****************************************************************************/
/* Set size group for internal labels                                       */
/****************************************************************************/
void
gl_wdgt_line_set_label_size_group (glWdgtLine   *line,
				   GtkSizeGroup *label_size_group)
{
	gtk_size_group_add_widget (label_size_group, line->width_label);
	gtk_size_group_add_widget (label_size_group, line->color_label);
}

