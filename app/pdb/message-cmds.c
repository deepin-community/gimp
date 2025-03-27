/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include "stamp-pdbgen.h"

#include <string.h>

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"
#include "plug-in/gimpplugin.h"
#include "plug-in/gimppluginmanager.h"

#include "gimppdb.h"
#include "gimpprocedure.h"
#include "internal-procs.h"

#include "gimp-intl.h"


static GimpValueArray *
message_invoker (GimpProcedure         *procedure,
                 Gimp                  *gimp,
                 GimpContext           *context,
                 GimpProgress          *progress,
                 const GimpValueArray  *args,
                 GError               **error)
{
  gboolean success = TRUE;
  const gchar *message;

  message = g_value_get_string (gimp_value_array_index (args, 0));

  if (success)
    {
      const gchar *domain = NULL;

      if (gimp->plug_in_manager->current_plug_in)
        domain = gimp_plug_in_get_undo_desc (gimp->plug_in_manager->current_plug_in);
      gimp_show_message (gimp, G_OBJECT (progress), GIMP_MESSAGE_WARNING,
                         domain, message);
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static GimpValueArray *
message_get_handler_invoker (GimpProcedure         *procedure,
                             Gimp                  *gimp,
                             GimpContext           *context,
                             GimpProgress          *progress,
                             const GimpValueArray  *args,
                             GError               **error)
{
  GimpValueArray *return_vals;
  gint handler = 0;

  handler = gimp->message_handler;

  return_vals = gimp_procedure_get_return_values (procedure, TRUE, NULL);
  g_value_set_enum (gimp_value_array_index (return_vals, 1), handler);

  return return_vals;
}

static GimpValueArray *
message_set_handler_invoker (GimpProcedure         *procedure,
                             Gimp                  *gimp,
                             GimpContext           *context,
                             GimpProgress          *progress,
                             const GimpValueArray  *args,
                             GError               **error)
{
  gboolean success = TRUE;
  gint handler;

  handler = g_value_get_enum (gimp_value_array_index (args, 0));

  if (success)
    {
      gimp->message_handler = handler;
    }

  return gimp_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

void
register_message_procs (GimpPDB *pdb)
{
  GimpProcedure *procedure;

  /*
   * gimp-message
   */
  procedure = gimp_procedure_new (message_invoker, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-message");
  gimp_procedure_set_static_help (procedure,
                                  "Displays a dialog box with a message.",
                                  "Displays a dialog box with a message. Useful for status or error reporting. The message must be in UTF-8 encoding.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Manish Singh",
                                         "Manish Singh",
                                         "1998");
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("message",
                                                       "message",
                                                       "Message to display in the dialog",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-message-get-handler
   */
  procedure = gimp_procedure_new (message_get_handler_invoker, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-message-get-handler");
  gimp_procedure_set_static_help (procedure,
                                  "Returns the current state of where warning messages are displayed.",
                                  "This procedure returns the way g_message warnings are displayed. They can be shown in a dialog box or printed on the console where gimp was started.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Manish Singh",
                                         "Manish Singh",
                                         "1998");
  gimp_procedure_add_return_value (procedure,
                                   g_param_spec_enum ("handler",
                                                      "handler",
                                                      "The current handler type",
                                                      GIMP_TYPE_MESSAGE_HANDLER_TYPE,
                                                      GIMP_MESSAGE_BOX,
                                                      GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * gimp-message-set-handler
   */
  procedure = gimp_procedure_new (message_set_handler_invoker, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (procedure),
                               "gimp-message-set-handler");
  gimp_procedure_set_static_help (procedure,
                                  "Controls where warning messages are displayed.",
                                  "This procedure controls how g_message warnings are displayed. They can be shown in a dialog box or printed on the console where gimp was started.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Manish Singh",
                                         "Manish Singh",
                                         "1998");
  gimp_procedure_add_argument (procedure,
                               g_param_spec_enum ("handler",
                                                  "handler",
                                                  "The new handler type",
                                                  GIMP_TYPE_MESSAGE_HANDLER_TYPE,
                                                  GIMP_MESSAGE_BOX,
                                                  GIMP_PARAM_READWRITE));
  gimp_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
