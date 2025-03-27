/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimphelp_pdb.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "stamp-pdbgen.h"

#include "gimp.h"


/**
 * SECTION: gimphelp
 * @title: gimphelp
 * @short_description: Loading help pages using gimp_help.
 *
 * Loading help pages using gimp_help.
 **/


/**
 * gimp_help:
 * @help_domain: (nullable): The help domain in which help_id is registered.
 * @help_id: The help page's ID.
 *
 * Load a help page.
 *
 * This procedure loads the specified help page into the helpbrowser or
 * what ever is configured as help viewer. The help page is identified
 * by its domain and ID: if help_domain is NULL, we use the help_domain
 * which was registered using the gimp_plugin_help_register()
 * procedure. If help_domain is NULL and no help domain was registered,
 * the help domain of the main GIMP installation is used.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_help (const gchar *help_domain,
           const gchar *help_id)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, help_domain,
                                          G_TYPE_STRING, help_id,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-help",
                                               args);
  gimp_value_array_unref (args);

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}
