/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselectbutton.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_PALETTE_CHOOSER_H__
#define __GIMP_PALETTE_CHOOSER_H__

#include <libgimp/gimpresourcechooser.h>

G_BEGIN_DECLS

#define GIMP_TYPE_PALETTE_CHOOSER (gimp_palette_chooser_get_type ())
G_DECLARE_FINAL_TYPE (GimpPaletteChooser, gimp_palette_chooser, GIMP, PALETTE_CHOOSER, GimpResourceChooser)


GtkWidget * gimp_palette_chooser_new (const gchar *title,
                                      const gchar *label,
                                      GimpPalette *palette);


G_END_DECLS

#endif /* __GIMP_PALETTE_CHOOSER_H__ */
