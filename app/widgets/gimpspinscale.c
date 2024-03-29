/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpspinscale.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
 *               2012 Øyvind Kolås    <pippin@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "gimpspinscale.h"

#include "gimp-intl.h"


#define RELATIVE_CHANGE_SPEED 0.1


enum
{
  PROP_0,
  PROP_LABEL
};

typedef enum
{
  TARGET_NONE,
  TARGET_NUMBER,
  TARGET_UPPER,
  TARGET_LOWER
} SpinScaleTarget;


typedef struct _GimpSpinScalePrivate GimpSpinScalePrivate;

struct _GimpSpinScalePrivate
{
  gboolean         compact;

  gchar           *label;
  gchar           *label_text;
  gchar           *label_pattern;

  GtkWindow       *mnemonic_window;
  guint            mnemonic_keyval;
  gboolean         mnemonics_visible;

  gboolean         constrain_drag;

  gboolean         scale_limits_set;
  gdouble          scale_lower;
  gdouble          scale_upper;
  gdouble          gamma;

  PangoLayout     *layout;
  gboolean         changing_value;
  gboolean         relative_change;
  gdouble          start_x;
  gdouble          start_value;
  GdkScreen       *start_screen;
  gint             start_pointer_x;
  gint             start_pointer_y;
  SpinScaleTarget  target;
  gboolean         hover;
  gboolean         pointer_warp;
  gint             pointer_warp_x;
  gint             pointer_warp_start_x;

  gint             change_value_idle_id;
  gdouble          change_value_idle_value;
};

#define GET_PRIVATE(obj) ((GimpSpinScalePrivate *) gimp_spin_scale_get_instance_private ((GimpSpinScale *) (obj)))


static void       gimp_spin_scale_dispose           (GObject          *object);
static void       gimp_spin_scale_finalize          (GObject          *object);
static void       gimp_spin_scale_set_property      (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void       gimp_spin_scale_get_property      (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);

static void       gimp_spin_scale_map               (GtkWidget        *widget);
static void       gimp_spin_scale_size_request      (GtkWidget        *widget,
                                                     GtkRequisition   *requisition);
static void       gimp_spin_scale_style_set         (GtkWidget        *widget,
                                                     GtkStyle         *prev_style);
static gboolean   gimp_spin_scale_expose            (GtkWidget        *widget,
                                                     GdkEventExpose   *event);
static gboolean   gimp_spin_scale_button_press      (GtkWidget        *widget,
                                                     GdkEventButton   *event);
static gboolean   gimp_spin_scale_button_release    (GtkWidget        *widget,
                                                     GdkEventButton   *event);
static gboolean   gimp_spin_scale_motion_notify     (GtkWidget        *widget,
                                                     GdkEventMotion   *event);
static gboolean   gimp_spin_scale_leave_notify      (GtkWidget        *widget,
                                                     GdkEventCrossing *event);
static void       gimp_spin_scale_hierarchy_changed (GtkWidget        *widget,
                                                     GtkWidget        *old_toplevel);
static void       gimp_spin_scale_screen_changed    (GtkWidget        *widget,
                                                     GdkScreen        *old_screen);

static void       gimp_spin_scale_value_changed     (GtkSpinButton    *spin_button);

static void       gimp_spin_scale_settings_notify   (GtkSettings      *settings,
                                                     const GParamSpec *pspec,
                                                     GimpSpinScale    *scale);
static void       gimp_spin_scale_mnemonics_notify  (GtkWindow        *window,
                                                     const GParamSpec *pspec,
                                                     GimpSpinScale    *scale);
static void       gimp_spin_scale_setup_mnemonic    (GimpSpinScale    *scale,
                                                     guint             previous_keyval);

static gdouble    odd_pow                           (gdouble           x,
                                                     gdouble           y);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSpinScale, gimp_spin_scale,
                            GIMP_TYPE_SPIN_BUTTON)

#define parent_class gimp_spin_scale_parent_class


static void
gimp_spin_scale_class_init (GimpSpinScaleClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GtkWidgetClass     *widget_class      = GTK_WIDGET_CLASS (klass);
  GtkSpinButtonClass *spin_button_class = GTK_SPIN_BUTTON_CLASS (klass);

  object_class->dispose              = gimp_spin_scale_dispose;
  object_class->finalize             = gimp_spin_scale_finalize;
  object_class->set_property         = gimp_spin_scale_set_property;
  object_class->get_property         = gimp_spin_scale_get_property;

  widget_class->map                  = gimp_spin_scale_map;
  widget_class->size_request         = gimp_spin_scale_size_request;
  widget_class->style_set            = gimp_spin_scale_style_set;
  widget_class->expose_event         = gimp_spin_scale_expose;
  widget_class->button_press_event   = gimp_spin_scale_button_press;
  widget_class->button_release_event = gimp_spin_scale_button_release;
  widget_class->motion_notify_event  = gimp_spin_scale_motion_notify;
  widget_class->leave_notify_event   = gimp_spin_scale_leave_notify;
  widget_class->hierarchy_changed    = gimp_spin_scale_hierarchy_changed;
  widget_class->screen_changed       = gimp_spin_scale_screen_changed;

  spin_button_class->value_changed   = gimp_spin_scale_value_changed;

  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("compact",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_PARAM_READABLE));
}

static void
gimp_spin_scale_init (GimpSpinScale *scale)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);

  gtk_widget_add_events (GTK_WIDGET (scale),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

  gtk_entry_set_alignment (GTK_ENTRY (scale), 1.0);
  gtk_entry_set_has_frame (GTK_ENTRY (scale), FALSE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (scale), TRUE);

  private->mnemonic_keyval = GDK_KEY_VoidSymbol;
  private->gamma           = 1.0;
}

static void
gimp_spin_scale_dispose (GObject *object)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);
  guint                 keyval;

  keyval = private->mnemonic_keyval;
  private->mnemonic_keyval = GDK_KEY_VoidSymbol;

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (object), keyval);

  g_clear_object (&private->layout);

  if (private->change_value_idle_id)
    {
      GtkAdjustment *adjustment;

      adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (object));

      g_source_remove (private->change_value_idle_id);

      private->change_value_idle_id = 0;

      gtk_adjustment_set_value (adjustment, private->change_value_idle_value);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_spin_scale_finalize (GObject *object)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->label,         g_free);
  g_clear_pointer (&private->label_text,    g_free);
  g_clear_pointer (&private->label_pattern, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_spin_scale_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      gimp_spin_scale_set_label (scale, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_spin_scale_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gimp_spin_scale_get_label (scale));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_spin_scale_map (GtkWidget *widget)
{
  GdkWindow *window;

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  window = gtk_entry_get_text_window (GTK_ENTRY (widget));

  if (window)
    {
      /* as per 2020, motion hints seem to be broken, at least on X: calling
       * gdk_event_request_motions() doesn't seem to generate further motion
       * events, causing motion events to be discarded, especially if the spin-
       * scale is tied to some costly operation, such as projection
       * invalidation, which blocks the main thread.
       *
       * to fix this, we simply avoid motion hints for the widget, and use an
       * idle for setting the spin-scale value in response to motion events, as
       * a form of ad-hoc motion compression.
       *
       * note that this isn't necessary with gtk3, which does its own motion
       * compression.
       */
      gdk_window_set_events (window,
                             gdk_window_get_events (window) &
                             ~GDK_POINTER_MOTION_HINT_MASK);
    }
}

static void
gimp_spin_scale_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  PangoContext         *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics     *metrics;

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  metrics = pango_context_get_metrics (context, style->font_desc,
                                       pango_context_get_language (context));

  if (! private->compact)
    {
      gint height;

      height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
                             pango_font_metrics_get_descent (metrics));

      requisition->height += height;
    }

  if (private->label)
    {
      gint char_width;
      gint digit_width;
      gint char_pixels;

      char_width = pango_font_metrics_get_approximate_char_width (metrics);
      digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
      char_pixels = PANGO_PIXELS (MAX (char_width, digit_width));

      /* ~3 chars for the ellipses */
      requisition->width += char_pixels * 3;
    }

  pango_font_metrics_unref (metrics);
}

static void
gimp_spin_scale_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  g_clear_object (&private->layout);

  gtk_widget_style_get (widget,
                        "compact", &private->compact,
                        NULL);
}

static PangoAttrList *
pattern_to_attrs (const gchar *text,
                  const gchar *pattern)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  const char    *p     = text;
  const char    *q     = pattern;
  const char    *start;

  while (TRUE)
    {
      while (*p && q && *q != '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }
      start = p;
      while (*p && *q && *q == '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }

      if (p > start)
        {
          PangoAttribute *attr = pango_attr_underline_new (PANGO_UNDERLINE_LOW);

          attr->start_index = start - text;
          attr->end_index   = p - text;

          pango_attr_list_insert (attrs, attr);
        }
      else
        break;
    }

  return attrs;
}

static gboolean
gimp_spin_scale_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  cairo_t              *cr;
  gboolean              rtl;
  gint                  w, h;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  cr = gdk_cairo_create (event->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);

  w = gdk_window_get_width (event->window);
  h = gdk_window_get_height (event->window);

  /* upper/lower halves highlight */
  if (! private->compact                                              &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)) &&
      gtk_widget_get_sensitive (widget)                               &&
      (private->target == TARGET_UPPER || private->target == TARGET_LOWER))
    {
      gint            window_width;
      gint            window_height;
      const GdkColor *color;
      gdouble         r, g, b, a;

      window_width  = gdk_window_get_width  (event->window);
      window_height = gdk_window_get_height (event->window);

      color = &style->text[gtk_widget_get_state (widget)];

      switch (private->target)
        {
        case TARGET_UPPER:
          cairo_rectangle (cr, 0, 0, window_width, window_height / 2);
          break;

        case TARGET_LOWER:
          cairo_rectangle (cr, 0, window_height / 2, window_width, (window_height + 1) / 2);
          break;

        default:
          break;
        }

      r = (gdouble) color->red   / 0xffff;
      g = (gdouble) color->green / 0xffff;
      b = (gdouble) color->blue  / 0xffff;
      a = 0.12 + 0.04 * MAX (r, MAX (g, b));

      if (private->changing_value)
        a *= 1.6;

      cairo_set_source_rgba (cr, r, g, b, a);

      cairo_fill (cr);
    }

  cairo_set_line_width (cr, 1.0);

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      /* let spinbutton-side line of rectangle disappear */
      if (rtl)
        cairo_rectangle (cr, -0.5, 0.5, w, h - 1.0);
      else
        cairo_rectangle (cr, 0.5, 0.5, w, h - 1.0);

      gdk_cairo_set_source_color (cr,
                                  &style->text_aa[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }
  else
    {
      /* let text-box-side line of rectangle disappear */
      if (rtl)
        cairo_rectangle (cr, 0.5, 0.5, w, h - 1.0);
      else
        cairo_rectangle (cr, -0.5, 0.5, w, h - 1.0);

      gdk_cairo_set_source_color (cr,
                                  &style->text_aa[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);

      if (rtl)
        cairo_rectangle (cr, 1.5, 1.5, w - 2.0, h - 3.0);
      else
        cairo_rectangle (cr, 0.5, 1.5, w - 2.0, h - 3.0);

      gdk_cairo_set_source_color (cr,
                                  &style->base[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }

  if (private->label &&
      gtk_widget_is_drawable (widget) &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      GtkRequisition requisition;
      GtkAllocation  allocation;
      PangoRectangle logical;
      gint           layout_offset_x;
      gint           layout_offset_y;
      GtkStateType   state;
      GdkColor       text_color;
      GdkColor       bar_text_color;
      gint           window_width;
      gint           window_height;
      gdouble        progress_fraction;
      gint           progress_x;
      gint           progress_y;
      gint           progress_width;
      gint           progress_height;

      GTK_WIDGET_CLASS (parent_class)->size_request (widget, &requisition);
      gtk_widget_get_allocation (widget, &allocation);

      if (! private->layout)
        {
          private->layout = gtk_widget_create_pango_layout (widget,
                                                            private->label_text);
          pango_layout_set_ellipsize (private->layout, PANGO_ELLIPSIZE_END);

          if (private->mnemonics_visible)
            {
              PangoAttrList *attrs;

              attrs = pattern_to_attrs (private->label_text,
                                        private->label_pattern);
              if (attrs)
                {
                  pango_layout_set_attributes (private->layout, attrs);
                  pango_attr_list_unref (attrs);
                }
            }
        }

      pango_layout_set_width (private->layout,
                              PANGO_SCALE *
                              (allocation.width - requisition.width));
      pango_layout_get_pixel_extents (private->layout, NULL, &logical);

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), NULL, &layout_offset_y);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        layout_offset_x = w - logical.width - 2;
      else
        layout_offset_x = 2;

      layout_offset_x -= logical.x;

      state = GTK_STATE_SELECTED;
      if (! gtk_widget_get_sensitive (widget))
        state = GTK_STATE_INSENSITIVE;
      text_color     = style->text[gtk_widget_get_state (widget)];
      bar_text_color = style->fg[state];

      window_width  = gdk_window_get_width  (event->window);
      window_height = gdk_window_get_height (event->window);

      progress_fraction = gtk_entry_get_progress_fraction (GTK_ENTRY (widget));

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        {
          progress_fraction = 1.0 - progress_fraction;

          progress_x      = window_width * progress_fraction;
          progress_y      = 0;
          progress_width  = window_width - progress_x;
          progress_height = window_height;
        }
      else
        {
          progress_x      = 0;
          progress_y      = 0;
          progress_width  = window_width * progress_fraction;
          progress_height = window_height;
        }

      cairo_save (cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle (cr, 0, 0, window_width, window_height);
      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);

      cairo_move_to (cr, layout_offset_x, layout_offset_y);
      gdk_cairo_set_source_color (cr, &text_color);
      pango_cairo_show_layout (cr, private->layout);

      cairo_restore (cr);

      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);

      cairo_move_to (cr, layout_offset_x, layout_offset_y);
      gdk_cairo_set_source_color (cr, &bar_text_color);
      pango_cairo_show_layout (cr, private->layout);
    }

  cairo_destroy (cr);

  return FALSE;
}

static SpinScaleTarget
gimp_spin_scale_get_target (GtkWidget      *widget,
                            gdouble         x,
                            gdouble         y,
                            GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkAllocation         allocation;
  PangoRectangle        logical;
  gint                  layout_x;
  gint                  layout_y;

  if (private->compact && ! event)
    return TARGET_UPPER;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_entry_get_layout_offsets (GTK_ENTRY (widget), &layout_x, &layout_y);
  pango_layout_get_pixel_extents (gtk_entry_get_layout (GTK_ENTRY (widget)),
                                  NULL, &logical);

  if (x >= layout_x && x < layout_x + logical.width  &&
      y >= layout_y && y < layout_y + logical.height &&
      (! private->compact                            ||
       (gtk_widget_has_focus (widget)                &&
        gdk_event_triggers_context_menu ((GdkEvent *) event))))
    {
      return TARGET_NUMBER;
    }

  if (private->compact)
    {
      switch (event->button)
        {
        case 1:
          if (event->state & GDK_SHIFT_MASK)
            return TARGET_LOWER;
          else
            return TARGET_UPPER;

        case 3:
          return TARGET_LOWER;

        default:
          return TARGET_NUMBER;
        }
    }
   else
    {
      if (y >= allocation.height / 2)
        return TARGET_LOWER;
      else
        return TARGET_UPPER;
    }
}

static void
gimp_spin_scale_update_target (GtkWidget      *widget,
                               GdkWindow      *window,
                               gdouble         x,
                               gdouble         y,
                               GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  SpinScaleTarget       target;

  target = gimp_spin_scale_get_target (widget, x, y, event);

  if (target != private->target)
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor  = NULL;

      private->target = target;

      switch (target)
        {
        case TARGET_NUMBER:
          cursor = gdk_cursor_new_for_display (display, GDK_XTERM);
          break;

        case TARGET_UPPER:
          cursor = gdk_cursor_new_for_display (display, GDK_SB_UP_ARROW);
          break;

        case TARGET_LOWER:
          cursor = gdk_cursor_new_for_display (display, GDK_SB_H_DOUBLE_ARROW);
          break;

        default:
          break;
        }

      gdk_window_set_cursor (window, cursor);

      gtk_widget_queue_draw (widget);

      if (cursor)
        gdk_cursor_unref (cursor);
    }
}

static void
gimp_spin_scale_clear_target (GtkWidget *widget,
                              GdkWindow *window)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  if (private->target != TARGET_NONE)
    {
      private->target = TARGET_NONE;

      gdk_window_set_cursor (window, NULL);

      gtk_widget_queue_draw (widget);
    }
}

static void
gimp_spin_scale_get_limits (GimpSpinScale *scale,
                            gdouble       *lower,
                            gdouble       *upper)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);

  if (private->scale_limits_set)
    {
      *lower = private->scale_lower;
      *upper = private->scale_upper;
    }
  else
    {
      GtkSpinButton *spin_button = GTK_SPIN_BUTTON (scale);
      GtkAdjustment *adjustment  = gtk_spin_button_get_adjustment (spin_button);

      *lower = gtk_adjustment_get_lower (adjustment);
      *upper = gtk_adjustment_get_upper (adjustment);
    }
}

static gboolean
gimp_spin_scale_change_value_idle (GimpSpinScale *scale)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkAdjustment        *adjustment;

  private->change_value_idle_id = 0;

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (scale));

  gtk_adjustment_set_value (adjustment, private->change_value_idle_value);

  return G_SOURCE_REMOVE;
}

static void
gimp_spin_scale_change_value (GtkWidget *widget,
                              gdouble    x,
                              guint      state,
                              gboolean   now)
{
  GimpSpinScalePrivate *private     = GET_PRIVATE (widget);
  GtkSpinButton        *spin_button = GTK_SPIN_BUTTON (widget);
  GtkAdjustment        *adjustment  = gtk_spin_button_get_adjustment (spin_button);
  GdkWindow            *text_window = gtk_entry_get_text_window (GTK_ENTRY (widget));
  gdouble               lower;
  gdouble               upper;
  gint                  width;
  gdouble               value;
  gint                  digits;
  gint                  power = 1;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (widget), &lower, &upper);

  width = gdk_window_get_width (text_window);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x = width - x;

  if (private->relative_change)
    {
      gdouble step;

      step = (upper - lower) / width * RELATIVE_CHANGE_SPEED;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        step *= x - (width - private->start_x);
      else
        step *= x - private->start_x;

      if (state & GDK_CONTROL_MASK)
        {
          gdouble page_inc = gtk_adjustment_get_page_increment (adjustment);

          step = RINT (step / page_inc) * page_inc;
        }

      value = private->start_value + step;
    }
  else
    {
      gdouble x0, x1;
      gdouble fraction;

      x0 = odd_pow (lower, 1.0 / private->gamma);
      x1 = odd_pow (upper, 1.0 / private->gamma);

      fraction = x / (gdouble) width;

      value = fraction * (x1 - x0) + x0;
      value = odd_pow (value, private->gamma);

      if (state & GDK_CONTROL_MASK)
        {
          gdouble page_inc = gtk_adjustment_get_page_increment (adjustment);

          value = RINT (value / page_inc) * page_inc;
        }
    }

  digits = gtk_spin_button_get_digits (spin_button);
  while (digits--)
    power *= 10;

  /*  round the value to the possible precision of the spinbutton, so
   *  a focus-out will not change the value again, causing inadvertend
   *  adjustment signals.
   */
  value *= power;
  value = RINT (value);
  value /= power;

  if (private->constrain_drag)
    value = rint (value);

  if (now)
    {
      if (private->change_value_idle_id)
        {
          g_source_remove (private->change_value_idle_id);

          private->change_value_idle_id = 0;
        }

      gtk_adjustment_set_value (adjustment, value);
    }
  else if (! private->change_value_idle_id)
    {
      private->change_value_idle_value = value;

      private->change_value_idle_id = g_idle_add_full (
        G_PRIORITY_DEFAULT + 1,
        (GSourceFunc) gimp_spin_scale_change_value_idle,
        widget, NULL);
    }
}

static gboolean
gimp_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  private->changing_value  = FALSE;
  private->relative_change = FALSE;
  private->pointer_warp    = FALSE;

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      gimp_spin_scale_update_target (widget, event->window,
                                     event->x, event->y, event);

      gtk_widget_queue_draw (widget);

      switch (private->target)
        {
        case TARGET_UPPER:
          private->changing_value = TRUE;

          gtk_widget_grab_focus (widget);

          gimp_spin_scale_change_value (widget, event->x, event->state, TRUE);

          return TRUE;

        case TARGET_LOWER:
          private->changing_value = TRUE;

          gtk_widget_grab_focus (widget);

          private->relative_change = TRUE;
          private->start_x = event->x;
          private->start_value = gtk_adjustment_get_value (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget)));

          private->start_screen    = gdk_event_get_screen ((GdkEvent *) event);
          private->start_pointer_x = floor (event->x_root);
          private->start_pointer_y = floor (event->y_root);

          return TRUE;

        default:
          break;
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
gimp_spin_scale_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  if (private->changing_value)
    {
      private->changing_value = FALSE;

      /* don't change the value if we're in the middle of a pointer warp, since
       * we didn't adjust start_x yet.  see the comment in
       * gimp_spin_scale_motion_notify().
       */
      if (! private->pointer_warp)
        gimp_spin_scale_change_value (widget, event->x, event->state, TRUE);

      if (private->relative_change)
        {
          gdk_display_warp_pointer (gdk_screen_get_display (private->start_screen),
                                    private->start_screen,
                                    private->start_pointer_x,
                                    private->start_pointer_y);
        }

      if (private->hover)
        gimp_spin_scale_update_target (widget, event->window,
                                       event->x, event->y, NULL);
      else
        gimp_spin_scale_clear_target (widget, event->window);

      gtk_widget_queue_draw (widget);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_spin_scale_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  gdk_event_request_motions (event);

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    private->hover = TRUE;

  if (private->changing_value)
    {
      GdkScreen    *screen;
      GdkDisplay   *display;
      gint          pointer_x;
      gint          pointer_y;
      gint          monitor;
      GdkRectangle  monitor_geometry;

      screen  = gdk_event_get_screen ((GdkEvent *) event);
      display = gdk_screen_get_display (screen);

      pointer_x = floor (event->x_root);
      pointer_y = floor (event->y_root);

      monitor = gdk_screen_get_monitor_at_point (screen, pointer_x, pointer_y);
      gdk_screen_get_monitor_geometry (screen, monitor, &monitor_geometry);

      /* when applying a relative change, we wrap the pointer around the left
       * and right edges of the current monitor, so that the adjustment is not
       * limited by the monitor geometry.  when the pointer reaches one of the
       * monitor edges, we move it one pixel away from the opposite edge, so
       * that it can be subsequently moved in the other direction, and adjust
       * start_x accordingly.
       *
       * unfortunately, we can't rely on gdk_display_warp_pointer() to actually
       * move the pointer (for example, it doesn't work on wayland), and
       * there's no easy way to tell whether the pointer moved or not.  in
       * particular, even when the pointer doesn't move, gdk still simulates a
       * motion event, and reports the "new" pointer position until a real
       * motion event occurs.
       *
       * in order not to erroneously adjust start_x when
       * gdk_display_warp_pointer() fails, we remember that we *tried* to warp
       * the pointer, and defer the actual adjustment of start_x until a future
       * motion event, where the pointer's x coordinate is different from the
       * one passed to gdk_display_warp_pointer().  when that happens, we
       * "guess" whether the pointer got warped or not by comparing its x
       * coordinate to the one passed to gdk_display_warp_pointer(): if their
       * difference is less than half the monitor width, then we assume the
       * pointer got warped (otherwise, the user must have very quickly moved
       * the mouse across half the screen.)  yes, this is an ugly ugly hack :)
       */

      if (private->pointer_warp)
        {
          if (pointer_x == private->pointer_warp_x)
            return TRUE;

          private->pointer_warp = FALSE;

          if (ABS (pointer_x - private->pointer_warp_x) < monitor_geometry.width / 2)
            private->start_x = private->pointer_warp_start_x;
        }

      /* change the value in an idle, as a form of motion compression, since we
       * don't use motion hints.  see the comment in gimp_spin_scale_map().
       */
      gimp_spin_scale_change_value (widget, event->x, event->state, FALSE);

      if (private->relative_change)
        {
          GtkAdjustment *adjustment;
          gdouble        value;
          gdouble        lower;
          gdouble        upper;

          adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));

          value = gtk_adjustment_get_value (adjustment);
          lower = gtk_adjustment_get_lower (adjustment);
          upper = gtk_adjustment_get_upper (adjustment);

          if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            {
              gdouble temp;

              value = -value;

              temp  = lower;
              lower = -upper;
              upper = -temp;
            }

          if (pointer_x <= monitor_geometry.x &&
              value > lower)
            {
              private->pointer_warp         = TRUE;
              private->pointer_warp_x       = (monitor_geometry.width - 1) + pointer_x - 1;
              private->pointer_warp_start_x = private->start_x + (monitor_geometry.width - 2);
            }
          else if (pointer_x >= monitor_geometry.x + (monitor_geometry.width - 1) &&
                   value < upper)
            {
              private->pointer_warp         = TRUE;
              private->pointer_warp_x       = pointer_x - (monitor_geometry.width - 1) + 1;
              private->pointer_warp_start_x = private->start_x - (monitor_geometry.width - 2);
            }

          if (private->pointer_warp)
            {
              gdk_display_warp_pointer (display,
                                        screen,
                                        private->pointer_warp_x,
                                        pointer_y);
            }
        }

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
      private->hover)
    {
      gimp_spin_scale_update_target (widget, event->window,
                                     event->x, event->y, NULL);
    }

  return FALSE;
}

static gboolean
gimp_spin_scale_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      private->hover = FALSE;

      if (! (event->state &
             (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
        {
          gimp_spin_scale_clear_target (widget, event->window);
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->leave_notify_event (widget, event);
}

static void
gimp_spin_scale_hierarchy_changed (GtkWidget *widget,
                                   GtkWidget *old_toplevel)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (widget),
                                  private->mnemonic_keyval);
}

static void
gimp_spin_scale_screen_changed (GtkWidget *widget,
                                GdkScreen *old_screen)
{
  GimpSpinScale        *scale   = GIMP_SPIN_SCALE (widget);
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkSettings          *settings;

  g_clear_object (&private->layout);

  if (old_screen)
    {
      settings = gtk_settings_get_for_screen (old_screen);

      g_signal_handlers_disconnect_by_func (settings,
                                            gimp_spin_scale_settings_notify,
                                            scale);
    }

  if (! gtk_widget_has_screen (widget))
    return;

  settings = gtk_widget_get_settings (widget);

  g_signal_connect (settings, "notify::gtk-enable-mnemonics",
                    G_CALLBACK (gimp_spin_scale_settings_notify),
                    scale);
  g_signal_connect (settings, "notify::gtk-enable-accels",
                    G_CALLBACK (gimp_spin_scale_settings_notify),
                    scale);

  gimp_spin_scale_settings_notify (settings, NULL, scale);
}

static void
gimp_spin_scale_value_changed (GtkSpinButton *spin_button)
{
  GimpSpinScalePrivate *private    = GET_PRIVATE (spin_button);
  GtkAdjustment        *adjustment = gtk_spin_button_get_adjustment (spin_button);
  gdouble               lower;
  gdouble               upper;
  gdouble               value;
  gdouble               x0, x1;
  gdouble               x;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (spin_button), &lower, &upper);

  value = CLAMP (gtk_adjustment_get_value (adjustment), lower, upper);

  x0 = odd_pow (lower, 1.0 / private->gamma);
  x1 = odd_pow (upper, 1.0 / private->gamma);
  x  = odd_pow (value, 1.0 / private->gamma);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   (x - x0) / (x1 - x0));
}

static void
gimp_spin_scale_settings_notify (GtkSettings      *settings,
                                 const GParamSpec *pspec,
                                 GimpSpinScale    *scale)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (scale));

  if (GTK_IS_WINDOW (toplevel))
    gimp_spin_scale_mnemonics_notify (GTK_WINDOW (toplevel), NULL, scale);
}

static void
gimp_spin_scale_mnemonics_notify (GtkWindow        *window,
                                  const GParamSpec *pspec,
                                  GimpSpinScale    *scale)
{
  GimpSpinScalePrivate *private           = GET_PRIVATE (scale);
  gboolean              mnemonics_visible = FALSE;
  gboolean              enable_mnemonics;
  gboolean              auto_mnemonics;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (scale)),
                "gtk-enable-mnemonics", &enable_mnemonics,
                "gtk-auto-mnemonics",   &auto_mnemonics,
                NULL);

  if (enable_mnemonics &&
      (! auto_mnemonics ||
       gtk_widget_is_sensitive (GTK_WIDGET (scale))))
    {
      g_object_get (window,
                    "mnemonics-visible", &mnemonics_visible,
                    NULL);
    }

  if (private->mnemonics_visible != mnemonics_visible)
    {
      private->mnemonics_visible = mnemonics_visible;

      g_clear_object (&private->layout);

      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

static void
gimp_spin_scale_setup_mnemonic (GimpSpinScale *scale,
                                guint          previous_keyval)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkWidget            *widget  = GTK_WIDGET (scale);
  GtkWidget            *toplevel;

  if (private->mnemonic_window)
    {
      g_signal_handlers_disconnect_by_func (private->mnemonic_window,
                                            gimp_spin_scale_mnemonics_notify,
                                            scale);

      gtk_window_remove_mnemonic (private->mnemonic_window,
                                  previous_keyval,
                                  widget);
      private->mnemonic_window = NULL;
    }

  toplevel = gtk_widget_get_toplevel (widget);

  if (gtk_widget_is_toplevel (toplevel) &&
      private->mnemonic_keyval != GDK_KEY_VoidSymbol)
    {
      gtk_window_add_mnemonic (GTK_WINDOW (toplevel),
                               private->mnemonic_keyval,
                               widget);
      private->mnemonic_window = GTK_WINDOW (toplevel);

      g_signal_connect (toplevel, "notify::mnemonics-visible",
                        G_CALLBACK (gimp_spin_scale_mnemonics_notify),
                        scale);
     }
}

static gdouble
odd_pow (gdouble x,
         gdouble y)
{
  if (x >= 0.0)
    return pow (x, y);
  else
    return -pow (-x, y);
}


/*  public functions  */

GtkWidget *
gimp_spin_scale_new (GtkAdjustment *adjustment,
                     const gchar   *label,
                     gint           digits)
{
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  return g_object_new (GIMP_TYPE_SPIN_SCALE,
                       "adjustment", adjustment,
                       "label",      label,
                       "digits",     digits,
                       NULL);
}

static gboolean
separate_uline_pattern (const gchar  *str,
                        guint        *accel_key,
                        gchar       **new_str,
                        gchar       **pattern)
{
  gboolean underscore;
  const gchar *src;
  gchar *dest;
  gchar *pattern_dest;

  *accel_key = GDK_KEY_VoidSymbol;
  *new_str = g_new (gchar, strlen (str) + 1);
  *pattern = g_new (gchar, g_utf8_strlen (str, -1) + 1);

  underscore = FALSE;

  src = str;
  dest = *new_str;
  pattern_dest = *pattern;

  while (*src)
    {
      gunichar c;
      const gchar *next_src;

      c = g_utf8_get_char (src);
      if (c == (gunichar)-1)
        {
          g_warning ("Invalid input string");
          g_free (*new_str);
          g_free (*pattern);

          return FALSE;
        }
      next_src = g_utf8_next_char (src);

      if (underscore)
        {
          if (c == '_')
            *pattern_dest++ = ' ';
          else
            {
              *pattern_dest++ = '_';
              if (*accel_key == GDK_KEY_VoidSymbol)
                *accel_key = gdk_keyval_to_lower (gdk_unicode_to_keyval (c));
            }

          while (src < next_src)
            *dest++ = *src++;

          underscore = FALSE;
        }
      else
        {
          if (c == '_')
            {
              underscore = TRUE;
              src = next_src;
            }
          else
            {
              while (src < next_src)
                *dest++ = *src++;

              *pattern_dest++ = ' ';
            }
        }
    }

  *dest = 0;
  *pattern_dest = 0;

  return TRUE;
}

void
gimp_spin_scale_set_label (GimpSpinScale *scale,
                           const gchar   *label)
{
  GimpSpinScalePrivate *private;
  guint                 accel_key = GDK_KEY_VoidSymbol;
  gchar                *text      = NULL;
  gchar                *pattern   = NULL;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  if (label == private->label)
    return;

  if (label && ! separate_uline_pattern (label, &accel_key, &text, &pattern))
    return;

  g_free (private->label);
  private->label = g_strdup (label);

  g_free (private->label_text);
  private->label_text = text; /* don't dup */

  g_free (private->label_pattern);
  private->label_pattern = pattern; /* don't dup */

  if (private->mnemonic_keyval != accel_key)
    {
      guint previous = private->mnemonic_keyval;

      private->mnemonic_keyval = accel_key;

      gimp_spin_scale_setup_mnemonic (scale, previous);
    }

  g_clear_object (&private->layout);

  gtk_widget_queue_resize (GTK_WIDGET (scale));

  g_object_notify (G_OBJECT (scale), "label");
}

const gchar *
gimp_spin_scale_get_label (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), NULL);

  return GET_PRIVATE (scale)->label;
}

void
gimp_spin_scale_set_scale_limits (GimpSpinScale *scale,
                                  gdouble        lower,
                                  gdouble        upper)
{
  GimpSpinScalePrivate *private;
  GtkSpinButton        *spin_button;
  GtkAdjustment        *adjustment;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private     = GET_PRIVATE (scale);
  spin_button = GTK_SPIN_BUTTON (scale);
  adjustment  = gtk_spin_button_get_adjustment (spin_button);

  g_return_if_fail (lower >= gtk_adjustment_get_lower (adjustment));
  g_return_if_fail (upper <= gtk_adjustment_get_upper (adjustment));

  private->scale_limits_set = TRUE;
  private->scale_lower      = lower;
  private->scale_upper      = upper;
  private->gamma            = 1.0;

  gimp_spin_scale_value_changed (spin_button);
}

void
gimp_spin_scale_unset_scale_limits (GimpSpinScale *scale)
{
  GimpSpinScalePrivate *private;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->scale_limits_set = FALSE;
  private->scale_lower      = 0.0;
  private->scale_upper      = 0.0;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gboolean
gimp_spin_scale_get_scale_limits (GimpSpinScale *scale,
                                  gdouble       *lower,
                                  gdouble       *upper)
{
  GimpSpinScalePrivate *private;

  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), FALSE);

  private = GET_PRIVATE (scale);

  if (lower)
    *lower = private->scale_lower;

  if (upper)
    *upper = private->scale_upper;

  return private->scale_limits_set;
}

void
gimp_spin_scale_set_gamma (GimpSpinScale *scale,
                           gdouble        gamma)
{
  GimpSpinScalePrivate *private;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->gamma = gamma;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gdouble
gimp_spin_scale_get_gamma (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), 1.0);

  return GET_PRIVATE (scale)->gamma;
}

/**
 * gimp_spin_scale_set_constrain_drag:
 * @scale: the #GimpSpinScale.
 * @constrain: whether constraining to integer values when dragging with
 *             pointer.
 *
 * If @constrain_drag is TRUE, dragging the scale with the pointer will
 * only result into integer values. It will still possible to set the
 * scale to fractional values (if the spin scale "digits" is above 0)
 * for instance with keyboard edit.
 */
void
gimp_spin_scale_set_constrain_drag (GimpSpinScale *scale,
                                    gboolean       constrain)
{
  GimpSpinScalePrivate *private;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->constrain_drag = constrain;
}

gboolean
gimp_spin_scale_get_constrain_drag (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), 1.0);

  return GET_PRIVATE (scale)->constrain_drag;
}
