/*
 *  (GLABELS) Label and Business Card Creation program for GNOME
 *
 *  prefs-dialog.c:  Preferences dialog module
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

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include "bonobo-mdi.h"
#include "prefs-dialog.h"
#include "prefs.h"
#include "glabels.h"
#include "wdgt-line.h"
#include "wdgt-fill.h"
#include "wdgt-text-props.h"
#include "debug.h"

/*========================================================*/
/* Private macros and constants.                          */
/*========================================================*/

#define US_LETTER "US Letter"
#define A4        "A4"

/*========================================================*/
/* Private types.                                         */
/*========================================================*/

struct _glPrefsDialogPrivate
{
	GtkWidget* categories_tree;

	GtkWidget* notebook;

	GtkTreeModel *categories_tree_model;

	/* Units page */
	GtkWidget	*units_points_radio;
	GtkWidget	*units_inches_radio;
	GtkWidget	*units_mm_radio;

	/* Page size page */
	GtkWidget	*page_size_us_letter_radio;
	GtkWidget	*page_size_a4_radio;

	/* Default text properties */
	GtkWidget       *text;

	/* Default line properties */
	GtkWidget       *line;

	/* Default fill properties */
	GtkWidget       *fill;
};

/*========================================================*/
/* Private globals.                                       */
/*========================================================*/

static GtkDialogClass* parent_class = NULL;

/*========================================================*/
/* Private function prototypes.                           */
/*========================================================*/

static void gl_prefs_dialog_class_init 	(glPrefsDialogClass *klass);
static void gl_prefs_dialog_init       	(glPrefsDialog *dlg);
static void gl_prefs_dialog_finalize   	(GObject *object);
static void gl_prefs_dialog_construct   (glPrefsDialog *dlg);

static void response_cb                 (glPrefsDialog *dialog,
					 gint response,
					 gpointer user_data);

static GtkWidget *locale_page (glPrefsDialog *dlg);
static GtkWidget *object_page (glPrefsDialog *dlg);

static update_locale_page_from_prefs (glPrefsDialog *dlg);
static update_object_page_from_prefs (glPrefsDialog *dlg);

static update_prefs_from_locale_page (glPrefsDialog *dlg);
static update_prefs_from_object_page (glPrefsDialog *dlg);


/*****************************************************************************/
/* Boilerplate object stuff.                                                 */
/*****************************************************************************/
GType
gl_prefs_dialog_get_type (void)
{
	static GType dialog_type = 0;

	if (!dialog_type)
    	{
      		static const GTypeInfo dialog_info =
      		{
			sizeof (glPrefsDialogClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gl_prefs_dialog_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (glPrefsDialog),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gl_prefs_dialog_init
      		};

     		dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						      "glPrefsDialog",
						      &dialog_info, 
						      0);
    	}

	return dialog_type;
}

static void
gl_prefs_dialog_class_init (glPrefsDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	gl_debug (DEBUG_PREFS, "");
	
  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gl_prefs_dialog_finalize;  	
}

static void
gl_prefs_dialog_init (glPrefsDialog *dlg)
{
	gl_debug (DEBUG_PREFS, "");

	dlg->private = g_new0 (glPrefsDialogPrivate, 1);
}

static void 
gl_prefs_dialog_finalize (GObject *object)
{
	glPrefsDialog* dlg;
	
	gl_debug (DEBUG_PREFS, "");

	g_return_if_fail (object != NULL);
	
   	dlg = GL_PREFS_DIALOG (object);

	g_return_if_fail (GL_IS_PREFS_DIALOG (dlg));
	g_return_if_fail (dlg->private != NULL);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (dlg->private);
}

/*****************************************************************************/
/* NEW preferences dialog.                                                   */
/*****************************************************************************/
GtkWidget*
gl_prefs_dialog_new (GtkWindow *parent)
{
	GtkWidget *dlg;

	gl_debug (DEBUG_PREFS, "");

	dlg = GTK_WIDGET (g_object_new (GL_TYPE_PREFS_DIALOG, NULL));

	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
	
	gl_prefs_dialog_construct (GL_PREFS_DIALOG(dlg));

	return dlg;
}

/*---------------------------------------------------------------------------*/
/* PRIVATE.  Construct composite widget.                                     */
/*---------------------------------------------------------------------------*/
static void
gl_prefs_dialog_construct (glPrefsDialog *dlg)
{
	GtkWidget *notebook, *wlabel, *wvbox, *wvbox1, *whbox, *wframe;
	GSList *radio_group = NULL;

	g_return_if_fail (GL_IS_PREFS_DIALOG (dlg));
	g_return_if_fail (dlg->private != NULL);

	gtk_dialog_add_button (GTK_DIALOG(dlg),
			       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_CLOSE);

	g_signal_connect(G_OBJECT (dlg), "response",
			 G_CALLBACK (response_cb), NULL);

	notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->vbox),
			    notebook, TRUE, TRUE, 0);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				  locale_page (dlg),
				  gtk_label_new (_("Locale")));

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				  object_page (dlg),
				  gtk_label_new (_("Object defaults")));

	update_locale_page_from_prefs (dlg);
	update_object_page_from_prefs (dlg);

        gtk_widget_show_all (GTK_DIALOG (dlg)->vbox);   

        gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
        gtk_window_set_title (GTK_WINDOW (dlg), _("Preferences"));
        gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
}

/*---------------------------------------------------------------------------*/
/* PRIVATE.  "Response" callback.                                            */
/*---------------------------------------------------------------------------*/
static void
response_cb (glPrefsDialog *dlg,
	     gint          response,
	     gpointer      user_data)
{
	gl_debug (DEBUG_VIEW, "START");

	g_return_if_fail(dlg != NULL);
	g_return_if_fail(GTK_IS_DIALOG(dlg));

	switch(response) {
	case GTK_RESPONSE_CLOSE:
		gtk_widget_hide (GTK_WIDGET(dlg));
		break;
	default:
		g_assert_not_reached();
	}

	gl_debug (DEBUG_VIEW, "END");
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Build Locale Properties Notebook Tab                           */
/*--------------------------------------------------------------------------*/
static GtkWidget *
locale_page (glPrefsDialog *dlg)
{
	GtkWidget *wlabel, *wvbox, *whbox, *wvbox1, *wframe;
	GSList *radio_group = NULL;

	wvbox = gtk_vbox_new (FALSE, GNOME_PAD);
	gtk_container_set_border_width (GTK_CONTAINER (wvbox), 10);

	wlabel = gtk_label_new (_("Select locale specific behavior."));
	gtk_box_pack_start (GTK_BOX (wvbox), wlabel, FALSE, FALSE, 0);

	whbox = gtk_hbox_new (TRUE, GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (wvbox), whbox, FALSE, FALSE, 0);

	/* ----- Display Units Frame ------------------------------------ */
	wframe = gtk_frame_new (_("Display units"));
	gtk_box_pack_start (GTK_BOX (whbox), wframe, FALSE, TRUE, 0);

	wvbox1 = gtk_vbox_new (FALSE, GNOME_PAD);
	gtk_container_set_border_width (GTK_CONTAINER (wvbox1), 10);
	gtk_container_add (GTK_CONTAINER (wframe), wvbox1);

	radio_group = NULL;

	dlg->private->units_points_radio =
	    gtk_radio_button_new_with_label (radio_group, _("Points"));
	radio_group =
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (dlg->private->units_points_radio));
	gtk_box_pack_start (GTK_BOX (wvbox1), dlg->private->units_points_radio,
			    FALSE, FALSE, 0);

	dlg->private->units_inches_radio =
	    gtk_radio_button_new_with_label (radio_group, _("Inches"));
	radio_group =
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (dlg->private->units_inches_radio));
	gtk_box_pack_start (GTK_BOX (wvbox1), dlg->private->units_inches_radio,
			    FALSE, FALSE, 0);

	dlg->private->units_mm_radio =
	    gtk_radio_button_new_with_label (radio_group, _("Millimeters"));
	radio_group =
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (dlg->private->units_mm_radio));
	gtk_box_pack_start (GTK_BOX (wvbox1), dlg->private->units_mm_radio,
			    FALSE, FALSE, 0);

	/* ----- Page Size Frame ------------------------------------ */
	wframe = gtk_frame_new (_("Default page size"));
	gtk_box_pack_start (GTK_BOX (whbox), wframe, FALSE, TRUE, 0);

	wvbox1 = gtk_vbox_new (FALSE, GNOME_PAD);
	gtk_container_set_border_width (GTK_CONTAINER (wvbox1), 10);
	gtk_container_add (GTK_CONTAINER (wframe), wvbox1);

	radio_group = NULL;

	dlg->private->page_size_us_letter_radio =
	    gtk_radio_button_new_with_label (radio_group, US_LETTER);
	radio_group =
	    gtk_radio_button_get_group (GTK_RADIO_BUTTON
				    (dlg->private->page_size_us_letter_radio));
	gtk_box_pack_start (GTK_BOX (wvbox1), dlg->private->page_size_us_letter_radio, FALSE,
			    FALSE, 0);

	dlg->private->page_size_a4_radio =
	    gtk_radio_button_new_with_label (radio_group, A4);
	radio_group =
	    gtk_radio_button_get_group (GTK_RADIO_BUTTON (dlg->private->page_size_a4_radio));
	gtk_box_pack_start (GTK_BOX (wvbox1), dlg->private->page_size_a4_radio,
			    FALSE, FALSE, 0);

	g_signal_connect_swapped (
		G_OBJECT(dlg->private->units_points_radio),
		"toggled", G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_connect_swapped (
		G_OBJECT(dlg->private->units_inches_radio),
		"toggled", G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_connect_swapped (
		G_OBJECT(dlg->private->units_mm_radio),
		"toggled", G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_connect_swapped (
		G_OBJECT(dlg->private->page_size_us_letter_radio),
		"toggled", G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_connect_swapped (
		G_OBJECT(dlg->private->page_size_a4_radio),
		"toggled", G_CALLBACK(update_prefs_from_locale_page), dlg);

	return wvbox;
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Build Default Object Properties Notebook Tab                   */
/*--------------------------------------------------------------------------*/
static GtkWidget *
object_page (glPrefsDialog *dlg)
{
	GtkWidget *wlabel, *wvbox, *whbox;

	wvbox = gtk_vbox_new (FALSE, GNOME_PAD);
	gtk_container_set_border_width (GTK_CONTAINER (wvbox), 10);

	wlabel = gtk_label_new (_("Select default properties for new objects."));
	gtk_box_pack_start (GTK_BOX (wvbox), wlabel, FALSE, FALSE, 0);

	/* text props entry */
	dlg->private->text =
		gl_wdgt_text_props_new ("Text");
	gtk_box_pack_start (GTK_BOX (wvbox), dlg->private->text,
			    FALSE, FALSE, 0);

	whbox = gtk_hbox_new (TRUE, GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (wvbox), whbox, FALSE, FALSE, 0);

	/* ------ Line box ------ */
	dlg->private->line = gl_wdgt_line_new (_("Line"));
	gtk_box_pack_start (GTK_BOX (whbox), dlg->private->line,
			    FALSE, TRUE, 0);

	/* ------ Fill box ------ */
	dlg->private->fill = gl_wdgt_fill_new (_("Fill"));
	gtk_box_pack_start (GTK_BOX (whbox), dlg->private->fill,
			    FALSE, TRUE, 0);

	g_signal_connect_swapped (G_OBJECT(dlg->private->text),
				  "changed",
				  G_CALLBACK(update_prefs_from_object_page),
				  dlg);
	g_signal_connect_swapped (G_OBJECT(dlg->private->line),
				  "changed",
				  G_CALLBACK(update_prefs_from_object_page),
				  dlg);
	g_signal_connect_swapped (G_OBJECT(dlg->private->fill),
				  "changed",
				  G_CALLBACK(update_prefs_from_object_page),
				  dlg);
	return wvbox;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Update locale page widgets from current prefs.                 */
/*--------------------------------------------------------------------------*/
static update_locale_page_from_prefs (glPrefsDialog *dlg)
{
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->units_points_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->units_inches_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->units_mm_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->page_size_us_letter_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->page_size_a4_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);

	switch (gl_prefs->units) {
	case GL_PREFS_UNITS_PTS:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(dlg->private->units_points_radio),
			TRUE);
		break;
	case GL_PREFS_UNITS_INCHES:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(dlg->private->units_inches_radio),
			TRUE);
		break;
	case GL_PREFS_UNITS_MM:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(dlg->private->units_mm_radio),
			TRUE);
		break;
	default:
		g_warning ("Illegal units");	/* Should not happen */
		break;
	}

	if ( g_strcasecmp(gl_prefs->default_page_size, US_LETTER) == 0) {
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(dlg->private->page_size_us_letter_radio),
			TRUE);
	} else if ( g_strcasecmp(gl_prefs->default_page_size, A4) == 0) {
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON(dlg->private->page_size_a4_radio),
			TRUE);
	} else {
		g_warning ("Unknown default page size"); /* Shouldn't happen */
	}

	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->units_points_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->units_inches_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->units_mm_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->page_size_us_letter_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->page_size_a4_radio),
		G_CALLBACK(update_prefs_from_locale_page), dlg);
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Update object page widgets from current prefs.                 */
/*--------------------------------------------------------------------------*/
static update_object_page_from_prefs (glPrefsDialog *dlg)
{
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->text),
		G_CALLBACK(update_prefs_from_object_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->line),
		G_CALLBACK(update_prefs_from_object_page), dlg);
	g_signal_handlers_block_by_func (
		G_OBJECT(dlg->private->fill),
		G_CALLBACK(update_prefs_from_object_page), dlg);

	gl_wdgt_text_props_set_params (GL_WDGT_TEXT_PROPS(dlg->private->text),
				       gl_prefs->default_font_family,
				       gl_prefs->default_font_size,
				       gl_prefs->default_font_weight,
				       gl_prefs->default_font_italic_flag,
				       gl_prefs->default_text_color,
				       gl_prefs->default_text_alignment);

	gl_wdgt_line_set_params (GL_WDGT_LINE(dlg->private->line),
				 gl_prefs->default_line_width,
				 gl_prefs->default_line_color);

	gl_wdgt_fill_set_params (GL_WDGT_FILL(dlg->private->fill),
				 gl_prefs->default_fill_color);

	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->text),
		G_CALLBACK(update_prefs_from_object_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->line),
		G_CALLBACK(update_prefs_from_object_page), dlg);
	g_signal_handlers_unblock_by_func (
		G_OBJECT(dlg->private->fill),
		G_CALLBACK(update_prefs_from_object_page), dlg);
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Update prefs from current state of locale page widgets.        */
/*--------------------------------------------------------------------------*/
static update_prefs_from_locale_page (glPrefsDialog *dlg)
{
	if (gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON(dlg->private->units_points_radio)) == 0) {
		gl_prefs->units = GL_PREFS_UNITS_PTS;
	}
	if (gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON(dlg->private->units_inches_radio)) == 0) {
		gl_prefs->units = GL_PREFS_UNITS_INCHES;
	}
	if (gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON(dlg->private->units_mm_radio)) == 0) {
		gl_prefs->units = GL_PREFS_UNITS_MM;
	}

	if (gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON(dlg->private->page_size_us_letter_radio)) == 0) {
		gl_prefs->default_page_size = US_LETTER;
	}
	if (gtk_toggle_button_get_active (
		    GTK_TOGGLE_BUTTON(dlg->private->page_size_a4_radio)) == 0) {
		gl_prefs->default_page_size = A4;
	}

	gl_prefs_save_settings ();
}

/*--------------------------------------------------------------------------*/
/* PRIVATE.  Update prefs from current state of object page widgets.        */
/*--------------------------------------------------------------------------*/
static update_prefs_from_object_page (glPrefsDialog *dlg)
{
	gl_wdgt_text_props_get_params (GL_WDGT_TEXT_PROPS(dlg->private->text),
				       &gl_prefs->default_font_family,
				       &gl_prefs->default_font_size,
				       &gl_prefs->default_font_weight,
				       &gl_prefs->default_font_italic_flag,
				       &gl_prefs->default_text_color,
				       &gl_prefs->default_text_alignment);

	gl_wdgt_line_get_params (GL_WDGT_LINE(dlg->private->line),
				 &gl_prefs->default_line_width,
				 &gl_prefs->default_line_color);

	gl_wdgt_fill_get_params (GL_WDGT_FILL(dlg->private->fill),
				 &gl_prefs->default_fill_color);

	gl_prefs_save_settings ();
}


	