/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgimprc_pdb.c
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
 * SECTION: gimpgimprc
 * @title: gimpgimprc
 * @short_description: Interactions with settings from gimprc.
 *
 * Interactions with settings from gimprc.
 **/


/**
 * gimp_gimprc_query:
 * @token: The token to query for.
 *
 * Queries the gimprc file parser for information on a specified token.
 *
 * This procedure is used to locate additional information contained in
 * the gimprc file considered extraneous to the operation of GIMP.
 * Plug-ins that need configuration information can expect it will be
 * stored in the user gimprc file and can use this procedure to
 * retrieve it. This query procedure will return the value associated
 * with the specified token. This corresponds _only_ to entries with
 * the format: (&lt;token&gt; &lt;value&gt;). The value must be a
 * string. Entries not corresponding to this format will cause warnings
 * to be issued on gimprc parsing and will not be queryable.
 *
 * Returns: (transfer full): The value associated with the queried token.
 *          The returned value must be freed with g_free().
 **/
gchar *
gimp_gimprc_query (const gchar *token)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *value = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, token,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-gimprc-query",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    value = GIMP_VALUES_DUP_STRING (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return value;
}

/**
 * gimp_gimprc_set:
 * @token: The token to add or modify.
 * @value: The value to set the token to.
 *
 * Sets a gimprc token to a value and saves it in the gimprc.
 *
 * This procedure is used to add or change additional information in
 * the gimprc file that is considered extraneous to the operation of
 * GIMP. Plug-ins that need configuration information can use this
 * function to store it, and gimp_gimprc_query() to retrieve it. This
 * will accept _only_ string values in UTF-8 encoding.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_gimprc_set (const gchar *token,
                 const gchar *value)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, token,
                                          G_TYPE_STRING, value,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-gimprc-set",
                                               args);
  gimp_value_array_unref (args);

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_get_default_comment:
 *
 * Get the default image comment as specified in the Preferences.
 *
 * Returns a copy of the default image comment.
 *
 * Returns: (transfer full): Default image comment.
 *          The returned value must be freed with g_free().
 **/
gchar *
gimp_get_default_comment (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *comment = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-get-default-comment",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    comment = GIMP_VALUES_DUP_STRING (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return comment;
}

/**
 * gimp_get_default_unit:
 *
 * Get the default unit (taken from the user's locale).
 *
 * Returns the default unit.
 *
 * Returns: (transfer none): Default unit.
 *
 * Since: 2.4
 **/
GimpUnit *
gimp_get_default_unit (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  GimpUnit *unit = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-get-default-unit",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    unit = GIMP_VALUES_GET_UNIT (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return unit;
}

/**
 * gimp_get_monitor_resolution:
 * @xres: (out): X resolution.
 * @yres: (out): Y resolution.
 *
 * Get the monitor resolution as specified in the Preferences.
 *
 * Returns the resolution of the monitor in pixels/inch. This value is
 * taken from the Preferences (or the windowing system if this is set
 * in the Preferences) and there's no guarantee for the value to be
 * reasonable.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_get_monitor_resolution (gdouble *xres,
                             gdouble *yres)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-get-monitor-resolution",
                                               args);
  gimp_value_array_unref (args);

  *xres = 0.0;
  *yres = 0.0;

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  if (success)
    {
      *xres = GIMP_VALUES_GET_DOUBLE (return_vals, 1);
      *yres = GIMP_VALUES_GET_DOUBLE (return_vals, 2);
    }

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * _gimp_get_color_configuration:
 *
 * Get a serialized version of the color management configuration.
 *
 * Returns a string that can be deserialized into a GimpColorConfig
 * object representing the current color management configuration.
 *
 * Returns: (transfer full): Serialized color management configuration.
 *          The returned value must be freed with g_free().
 *
 * Since: 2.4
 **/
gchar *
_gimp_get_color_configuration (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *config = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-get-color-configuration",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    config = GIMP_VALUES_DUP_STRING (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return config;
}

/**
 * gimp_get_module_load_inhibit:
 *
 * Get the list of modules which should not be loaded.
 *
 * Returns a copy of the list of modules which should not be loaded.
 *
 * Returns: (transfer full): The list of modules.
 *          The returned value must be freed with g_free().
 **/
gchar *
gimp_get_module_load_inhibit (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *load_inhibit = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-get-module-load-inhibit",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    load_inhibit = GIMP_VALUES_DUP_STRING (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return load_inhibit;
}
