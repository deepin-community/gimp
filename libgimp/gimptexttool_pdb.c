/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimptexttool_pdb.c
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
 * SECTION: gimptexttool
 * @title: gimptexttool
 * @short_description: Functions for controlling the text tool.
 *
 * Functions for controlling the text tool.
 **/


/**
 * gimp_text_font:
 * @image: The image.
 * @drawable: (nullable): The affected drawable: (%NULL for a new text layer).
 * @x: The x coordinate for the left of the text bounding box.
 * @y: The y coordinate for the top of the text bounding box.
 * @text: The text to generate (in UTF-8 encoding).
 * @border: The size of the border.
 * @antialias: Antialiasing.
 * @size: The size of text in pixels.
 * @font: The font.
 *
 * Add text at the specified location as a floating selection or a new
 * layer.
 *
 * The x and y parameters together control the placement of the new
 * text by specifying the upper left corner of the text bounding box.
 * If the specified drawable parameter is valid, the text will be
 * created as a floating selection attached to the drawable. If the
 * drawable parameter is not valid (%NULL), the text will appear as a
 * new layer. Finally, a border can be specified around the final
 * rendered text. The border is measured in pixels.
 * The size is always in pixels. If you need to display a font in
 * points, divide the size in points by 72.0 and multiply it by the
 * image's vertical resolution.
 *
 * Returns: (nullable) (transfer none):
 *          The new text layer or %NULL if no layer was created.
 **/
GimpLayer *
gimp_text_font (GimpImage    *image,
                GimpDrawable *drawable,
                gdouble       x,
                gdouble       y,
                const gchar  *text,
                gint          border,
                gboolean      antialias,
                gdouble       size,
                GimpFont     *font)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  GimpLayer *text_layer = NULL;

  args = gimp_value_array_new_from_types (NULL,
                                          GIMP_TYPE_IMAGE, image,
                                          GIMP_TYPE_DRAWABLE, drawable,
                                          G_TYPE_DOUBLE, x,
                                          G_TYPE_DOUBLE, y,
                                          G_TYPE_STRING, text,
                                          G_TYPE_INT, border,
                                          G_TYPE_BOOLEAN, antialias,
                                          G_TYPE_DOUBLE, size,
                                          GIMP_TYPE_FONT, font,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-text-font",
                                               args);
  gimp_value_array_unref (args);

  if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
    text_layer = GIMP_VALUES_GET_LAYER (return_vals, 1);

  gimp_value_array_unref (return_vals);

  return text_layer;
}

/**
 * gimp_text_get_extents_font:
 * @text: The text to generate (in UTF-8 encoding).
 * @size: The size of text in either pixels or points.
 * @font: The name of the font.
 * @width: (out): The width of the glyph extents.
 * @height: (out): The height of the glyph extents.
 * @ascent: (out): The ascent of the glyph extents.
 * @descent: (out): The descent of the glyph extents.
 *
 * Get extents of the bounding box for the specified text.
 *
 * This tool returns the width and height of a bounding box for the
 * specified text rendered with the specified font information. Ascent
 * and descent of the glyph extents are returned as well.
 * The ascent is the distance from the baseline to the highest point of
 * the character. This is positive if the glyph ascends above the
 * baseline. The descent is the distance from the baseline to the
 * lowest point of the character. This is positive if the glyph
 * descends below the baseline.
 * The size is always in pixels. If you need to set a font in points,
 * divide the size in points by 72.0 and multiply it by the vertical
 * resolution of the image you are taking into account.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_text_get_extents_font (const gchar *text,
                            gdouble      size,
                            GimpFont    *font,
                            gint        *width,
                            gint        *height,
                            gint        *ascent,
                            gint        *descent)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, text,
                                          G_TYPE_DOUBLE, size,
                                          GIMP_TYPE_FONT, font,
                                          G_TYPE_NONE);

  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-text-get-extents-font",
                                               args);
  gimp_value_array_unref (args);

  *width = 0;
  *height = 0;
  *ascent = 0;
  *descent = 0;

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  if (success)
    {
      *width = GIMP_VALUES_GET_INT (return_vals, 1);
      *height = GIMP_VALUES_GET_INT (return_vals, 2);
      *ascent = GIMP_VALUES_GET_INT (return_vals, 3);
      *descent = GIMP_VALUES_GET_INT (return_vals, 4);
    }

  gimp_value_array_unref (return_vals);

  return success;
}
