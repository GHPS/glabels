/*
 *  (GLABELS) Label and Business Card Creation program for GNOME
 *
 *  glabels.c: main program module
 *
 *  Copyright (C) 2001  Jim Evins <evins@snaught.com>.
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

#include <gnome.h>
#include <libgnomeprint/gnome-print-job.h>

#include "merge-init.h"
#include "xml-label.h"
#include "print.h"
#include <libglabels/paper.h>
#include <libglabels/template.h>
#include "util.h"

/*============================================*/
/* Private globals                            */
/*============================================*/
static gboolean help_flag    = FALSE;
static gboolean version_flag = FALSE;
static gchar    *output      = "output.ps";
static gint     n_copies     = 1;
static gint     n_sheets     = 1;
static gboolean outline_flag = FALSE;
static gboolean reverse_flag = FALSE;

static struct poptOption options[] = {
	{"help", 'h', POPT_ARG_NONE, &help_flag, 1,
	 N_("print this message"), NULL},
	{"version", 'v', POPT_ARG_NONE, &version_flag, 0,
	 N_("print the version of glabels-batch being used"), NULL},
	{"output", 'o', POPT_ARG_STRING, &output, 0,
	 N_("set output filename (default=\"output.ps\")"), N_("filename")},
	{"sheets", 's', POPT_ARG_INT, &n_sheets, 0,
	 N_("number of sheets (default=1)"), N_("sheets")},
	{"copies", 'c', POPT_ARG_INT, &n_copies, 0,
	 N_("number of copies (default=1)"), N_("copies")},
	{"outline", 'l', POPT_ARG_NONE, &outline_flag, 0,
	 N_("print outlines (to test printer alignment)"), NULL},
	{"reverse", 'r', POPT_ARG_NONE, &reverse_flag, 0,
	 N_("print in reverse (i.e. a mirror image)"), NULL},
	{NULL, '\0', 0, NULL, 0, NULL, NULL}
};



/*****************************************************************************/
/* Main                                                                      */
/*****************************************************************************/
int
main (int argc, char **argv)
{
    	GnomeProgram      *program;
	poptContext        pctx;
	gchar            **args;
	gint               rc;
	GSList            *p, *file_list = NULL;
	gint               n_files;
	GnomePrintJob     *job = NULL;
	gchar             *abs_fn;
	GnomePrintConfig  *config = NULL;
	glLabel           *label = NULL;
	glXMLLabelStatus   status;
	glPrintFlags       flags;

	bindtextdomain (GETTEXT_PACKAGE, GLABELS_LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	/* Initialize minimal gnome program */
	program = gnome_program_init ("glabels-batch", VERSION,
				      LIBGNOME_MODULE, 1, argv,
				      GNOME_PROGRAM_STANDARD_PROPERTIES,
				      NULL);

	/* argument parsing */
	pctx = poptGetContext (NULL, argc, (const char **)argv, options, 0);
	poptSetOtherOptionHelp (pctx, _("[OPTION...] GLABELS_FILE...") );
	if ( (rc = poptGetNextOpt(pctx)) < -1 ) {
		fprintf (stderr, "%s: %s\n",
			 poptBadOption (pctx,0), poptStrerror(rc));
		poptPrintUsage (pctx, stderr, 0);
		return -1;
	}
	if ( version_flag ) {
		fprintf ( stderr, "glabels-batch %s\n", VERSION );
		return -1;
	}
	if ( help_flag ) {
		poptPrintHelp (pctx, stderr, 0);
		return -1;
	}
	args = (char **) poptGetArgs (pctx);
	for (n_files = 0; args && args[n_files]; n_files++) {
		file_list = g_slist_append (file_list, args[n_files]);
	}
	if ( !n_files ) {
		fprintf ( stderr, _("missing glabels file\n") );
		poptPrintHelp (pctx, stderr, 0);
		return -1;
	}
	poptFreeContext (pctx);

	flags.outline = outline_flag;
	flags.reverse = reverse_flag;
	flags.crop_marks = FALSE;

	/* initialize components */
	gl_merge_init ();
	gl_paper_init ();
	gl_template_init ();

	/* now print the files */
	for (p = file_list; p; p = p->next) {
		g_print ("LABEL FILE = %s\n", p->data);
		label = gl_xml_label_open (p->data, &status);
		if ( status == XML_LABEL_OK ) {

			if ( job == NULL ) {
				job = gnome_print_job_new (NULL);
				abs_fn = gl_util_make_absolute ( output );
				config = gnome_print_job_get_config (job);
				gnome_print_config_set (config,
							"Printer",
							"GENERIC");
				gnome_print_config_set (config,
							"Settings.Transport.Backend",
							"file");
				gnome_print_config_set (config,
							GNOME_PRINT_KEY_OUTPUT_FILENAME,
							abs_fn);
				g_free( abs_fn );
			}

			gl_print_batch (job, label, n_sheets, n_copies, &flags);
			g_object_unref (label);
		}
		else {
			fprintf ( stderr, _("cannot open glabels file %s\n"),
				  (char *)p->data );
		}
	}
	if ( job != NULL ) {

		gnome_print_job_close (job);
		gnome_print_job_print (job);

		g_object_unref (job);
	}

	g_slist_free (file_list);

	return 0;
}

