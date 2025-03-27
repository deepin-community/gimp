/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns-data.h
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#ifndef _ICNS_DATA_H
#define _ICNS_DATA_H

typedef struct _IconType
{
  gchar    *type;
  guint     width;
  guint     height;
  guint     bits;
  gchar    *mask;
  gboolean  isModern; /* OSX icns files */
} IconType;

extern IconType iconTypes[];
extern IconType maskTypes[];
extern guchar   icns_colormap_4[];
extern guchar   icns_colormap_8[];

#endif
