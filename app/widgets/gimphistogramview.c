/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimphistogram.h"
#include "core/gimpmarshal.h"

#include "gimphistogramview.h"


#define MIN_WIDTH  64
#define MIN_HEIGHT 64

enum
{
  RANGE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_SCALE,
  PROP_BORDER_WIDTH,
  PROP_SUBDIVISIONS
};


static void     gimp_histogram_view_dispose        (GObject        *object);
static void     gimp_histogram_view_set_property   (GObject        *object,
                                                    guint           property_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void     gimp_histogram_view_get_property   (GObject        *object,
                                                    guint           property_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);

static void     gimp_histogram_view_size_request   (GtkWidget      *widget,
                                                    GtkRequisition *requisition);
static gboolean gimp_histogram_view_expose         (GtkWidget      *widget,
                                                    GdkEventExpose *event);
static gboolean gimp_histogram_view_button_press   (GtkWidget      *widget,
                                                    GdkEventButton *bevent);
static gboolean gimp_histogram_view_button_release (GtkWidget      *widget,
                                                    GdkEventButton *bevent);
static gboolean gimp_histogram_view_motion_notify  (GtkWidget      *widget,
                                                    GdkEventMotion *bevent);

static void     gimp_histogram_view_notify      (GimpHistogram        *histogram,
                                                 const GParamSpec     *pspec,
                                                 GimpHistogramView    *view);
static void     gimp_histogram_view_update_bins (GimpHistogramView    *view);

static void     gimp_histogram_view_draw_spike  (GimpHistogramView    *view,
                                                 GimpHistogramChannel  channel,
                                                 cairo_t              *cr,
                                                 const GdkColor       *fg_color,
                                                 cairo_operator_t      fg_operator,
                                                 const GdkColor       *bg_color,
                                                 gint                  x,
                                                 gint                  i,
                                                 gint                  j,
                                                 gdouble               max,
                                                 gdouble               bg_max,
                                                 gint                  height,
                                                 gint                  border);


G_DEFINE_TYPE (GimpHistogramView, gimp_histogram_view,
               GTK_TYPE_DRAWING_AREA)

#define parent_class gimp_histogram_view_parent_class

static guint histogram_view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_histogram_view_class_init (GimpHistogramViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  histogram_view_signals[RANGE_CHANGED] =
    g_signal_new ("range-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpHistogramViewClass, range_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->dispose              = gimp_histogram_view_dispose;
  object_class->get_property         = gimp_histogram_view_get_property;
  object_class->set_property         = gimp_histogram_view_set_property;

  widget_class->size_request         = gimp_histogram_view_size_request;
  widget_class->expose_event         = gimp_histogram_view_expose;
  widget_class->button_press_event   = gimp_histogram_view_button_press;
  widget_class->button_release_event = gimp_histogram_view_button_release;
  widget_class->motion_notify_event  = gimp_histogram_view_motion_notify;

  klass->range_changed               = NULL;

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_enum ("histogram-channel",
                                                      NULL, NULL,
                                                      GIMP_TYPE_HISTOGRAM_CHANNEL,
                                                      GIMP_HISTOGRAM_VALUE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SCALE,
                                   g_param_spec_enum ("histogram-scale",
                                                      NULL, NULL,
                                                      GIMP_TYPE_HISTOGRAM_SCALE,
                                                      GIMP_HISTOGRAM_SCALE_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BORDER_WIDTH,
                                   g_param_spec_int ("border-width", NULL, NULL,
                                                     0, 32, 1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SUBDIVISIONS,
                                   g_param_spec_int ("subdivisions",
                                                     NULL, NULL,
                                                     1, 64, 5,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_histogram_view_init (GimpHistogramView *view)
{
  view->histogram    = NULL;
  view->bg_histogram = NULL;
  view->n_bins       = 256;
  view->start        = 0;
  view->end          = 255;
}

static void
gimp_histogram_view_dispose (GObject *object)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  gimp_histogram_view_set_histogram (view, NULL);
  gimp_histogram_view_set_background (view, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_histogram_view_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      view->channel = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;
    case PROP_SCALE:
      view->scale = g_value_get_enum (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;
    case PROP_BORDER_WIDTH:
      view->border_width = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (view));
      break;
    case PROP_SUBDIVISIONS:
      view->subdivisions = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (view));
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_view_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, view->channel);
      break;
    case PROP_SCALE:
      g_value_set_enum (value, view->scale);
      break;
    case PROP_BORDER_WIDTH:
      g_value_set_int (value, view->border_width);
      break;
    case PROP_SUBDIVISIONS:
      g_value_set_int (value, view->subdivisions);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_histogram_view_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  requisition->width  = MIN_WIDTH  + 2 * view->border_width;
  requisition->height = MIN_HEIGHT + 2 * view->border_width;
}

static gdouble
gimp_histogram_view_get_maximum (GimpHistogramView    *view,
                                 GimpHistogram        *histogram,
                                 GimpHistogramChannel  channel)
{
  gdouble max = gimp_histogram_get_maximum (histogram, channel);

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
      if (max > 0.0)
        max = log (max);
      else
        max = 1.0;
      break;
    }

  return max;
}

static gboolean
gimp_histogram_view_expose (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  GimpHistogramView *view  = GIMP_HISTOGRAM_VIEW (widget);
  GtkStyle          *style = gtk_widget_get_style (widget);
  GtkAllocation      allocation;
  cairo_t           *cr;
  gint               x;
  gint               x1, x2;
  gint               border;
  gint               width, height;
  gdouble            max    = 0.0;
  gdouble            bg_max = 0.0;
  gint               xstop;
  GdkColor          *color_in;
  GdkColor          *color_out;
  GdkColor          *bg_color_in;
  GdkColor          *bg_color_out;
  GdkColor           rgb_color[3];

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  /*  Draw the background  */
  gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_NORMAL]);
  cairo_paint (cr);

  gtk_widget_get_allocation (widget, &allocation);

  border = view->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_translate (cr, 0.5, 0.5);

  /*  Draw the outer border  */
  gdk_cairo_set_source_color (cr, &style->text_aa[GTK_STATE_NORMAL]);
  cairo_rectangle (cr, border, border,
                   width - 1, height - 1);
  cairo_stroke (cr);

  if (! view->histogram && ! view->bg_histogram)
    {
      cairo_destroy (cr);
      return FALSE;
    }

  x1 = CLAMP (MIN (view->start, view->end), 0, view->n_bins - 1);
  x2 = CLAMP (MAX (view->start, view->end), 0, view->n_bins - 1);

  if (view->histogram)
    max = gimp_histogram_view_get_maximum (view, view->histogram,
                                           view->channel);

  if (view->bg_histogram)
    bg_max = gimp_histogram_view_get_maximum (view, view->bg_histogram,
                                              view->channel);

  color_in  = &style->text[GTK_STATE_SELECTED];
  color_out = &style->text[GTK_STATE_NORMAL];

  bg_color_in  = &style->mid[GTK_STATE_SELECTED];
  bg_color_out = &style->mid[GTK_STATE_NORMAL];

  if (view->channel == GIMP_HISTOGRAM_RGB)
    {
      for (x = 0; x < 3; x++)
        {
          rgb_color[x].red   = (x == 0 ? 0xFFFF : 0x0);
          rgb_color[x].green = (x == 1 ? 0xFFFF : 0x0);
          rgb_color[x].blue  = (x == 2 ? 0xFFFF : 0x0);
        }
    }

  xstop = 1;
  for (x = 0; x < width; x++)
    {
      gboolean  in_selection = FALSE;

      gint  i = (x * view->n_bins) / width;
      gint  j = ((x + 1) * view->n_bins) / width;

      if (! (x1 == 0 && x2 == (view->n_bins - 1)))
        {
          gint k = i;

          do
            in_selection |= (x1 <= k && k <= x2);
          while (++k < j);
        }

      if (view->subdivisions > 1 && x >= (xstop * width / view->subdivisions))
        {
          gdk_cairo_set_source_color (cr, &style->text_aa[GTK_STATE_NORMAL]);

          cairo_move_to (cr, x + border, border);
          cairo_line_to (cr, x + border, border + height - 1);
          cairo_stroke (cr);

          xstop++;
        }
      else if (in_selection)
        {
          gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_SELECTED]);

          cairo_move_to (cr, x + border, border);
          cairo_line_to (cr, x + border, border + height - 1);
          cairo_stroke (cr);
        }

      if (view->channel == GIMP_HISTOGRAM_RGB)
        {
          gint c;

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c, cr,
                                            &style->black,
                                            CAIRO_OPERATOR_OVER,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          for (c = 0; c < 3; c++)
            gimp_histogram_view_draw_spike (view, GIMP_HISTOGRAM_RED + c, cr,
                                            &rgb_color[c],
                                            CAIRO_OPERATOR_ADD,
                                            NULL,
                                            x, i, j, max, bg_max, height, border);

          gimp_histogram_view_draw_spike (view, view->channel, cr,
                                          in_selection ? color_in : color_out,
                                          CAIRO_OPERATOR_OVER,
                                          NULL,
                                          x, i, j, max, bg_max, height, border);
        }
      else
        {
          gimp_histogram_view_draw_spike (view, view->channel, cr,
                                          in_selection ? color_in : color_out,
                                          CAIRO_OPERATOR_OVER,
                                          in_selection ? bg_color_in : bg_color_out,
                                          x, i, j, max, bg_max, height, border);
        }
    }

  cairo_destroy (cr);

  return FALSE;
}

static void
gimp_histogram_view_draw_spike (GimpHistogramView    *view,
                                GimpHistogramChannel  channel,
                                cairo_t              *cr,
                                const GdkColor       *fg_color,
                                cairo_operator_t      fg_operator,
                                const GdkColor       *bg_color,
                                gint                  x,
                                gint                  i,
                                gint                  j,
                                gdouble               max,
                                gdouble               bg_max,
                                gint                  height,
                                gint                  border)
{
  gdouble value    = 0.0;
  gdouble bg_value = 0.0;
  gint    y;
  gint    bg_y;

  if (view->histogram)
    {
      gint ii = i;

      do
        {
          gdouble v = gimp_histogram_get_value (view->histogram,
                                                channel, ii++);

          if (v > value)
            value = v;
        }
      while (ii < j);
    }

  if (bg_color && view->bg_histogram)
    {
      gint ii = i;

      do
        {
          gdouble v = gimp_histogram_get_value (view->bg_histogram,
                                                channel, ii++);

          if (v > bg_value)
            bg_value = v;
        }
      while (ii < j);
    }

  if (value <= 0.0 && bg_value <= 0.0)
    return;

  switch (view->scale)
    {
    case GIMP_HISTOGRAM_SCALE_LINEAR:
      y    = (gint) (((height - 2) * value)    / max);
      bg_y = (gint) (((height - 2) * bg_value) / bg_max);
      break;

    case GIMP_HISTOGRAM_SCALE_LOGARITHMIC:
      y    = (gint) (((height - 2) * log (value))    / max);
      bg_y = (gint) (((height - 2) * log (bg_value)) / bg_max);
      break;

    default:
      y    = 0;
      bg_y = 0;
      break;
    }

  y    = MAX (y,    0);
  bg_y = MAX (bg_y, 0);

  if (bg_color)
    {
      gdk_cairo_set_source_color (cr, bg_color);

      cairo_move_to (cr, x + border, height + border - 1);
      cairo_line_to (cr, x + border, height + border - bg_y - 1);

      cairo_stroke (cr);
    }

  cairo_set_operator (cr, fg_operator);

  gdk_cairo_set_source_color (cr, fg_color);

  cairo_move_to (cr, x + border, height + border - 1);
  cairo_line_to (cr, x + border, height + border - y - 1);

  cairo_stroke (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
}

static gboolean
gimp_histogram_view_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      GtkAllocation allocation;
      gint          width;

      gtk_grab_add (widget);

      gtk_widget_get_allocation (widget, &allocation);

      width = allocation.width - 2 * view->border_width;

      view->start = CLAMP (((bevent->x - view->border_width) * view->n_bins) / width,
                           0, view->n_bins - 1);
      view->end   = view->start;

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}

static gboolean
gimp_histogram_view_button_release (GtkWidget      *widget,
                                    GdkEventButton *bevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);

  if (bevent->button == 1)
    {
      gint start, end;

      gtk_grab_remove (widget);

      start = view->start;
      end   = view->end;

      view->start = MIN (start, end);
      view->end   = MAX (start, end);

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }

  return TRUE;
}

static gboolean
gimp_histogram_view_motion_notify (GtkWidget      *widget,
                                   GdkEventMotion *mevent)
{
  GimpHistogramView *view = GIMP_HISTOGRAM_VIEW (widget);
  GtkAllocation      allocation;
  gint               width;

  gtk_widget_get_allocation (widget, &allocation);

  width = allocation.width - 2 * view->border_width;

  view->start = CLAMP (((mevent->x - view->border_width) * view->n_bins) / width,
                       0, view->n_bins - 1);

  gtk_widget_queue_draw (widget);

  return TRUE;
}


/*  public functions  */

GtkWidget *
gimp_histogram_view_new (gboolean range)
{
  GtkWidget *view = g_object_new (GIMP_TYPE_HISTOGRAM_VIEW, NULL);

  if (range)
    gtk_widget_add_events (view,
                           GDK_BUTTON_PRESS_MASK   |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_BUTTON1_MOTION_MASK);

  return view;
}

void
gimp_histogram_view_set_histogram (GimpHistogramView *view,
                                   GimpHistogram     *histogram)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->bg_histogram == NULL ||
                    gimp_histogram_n_components (view->bg_histogram) ==
                    gimp_histogram_n_components (histogram));
#endif

  if (view->histogram != histogram)
    {
      if (view->histogram)
        {
          g_signal_handlers_disconnect_by_func (view->histogram,
                                                gimp_histogram_view_notify,
                                                view);
          g_object_unref (view->histogram);
        }

      view->histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          g_signal_connect (histogram, "notify",
                            G_CALLBACK (gimp_histogram_view_notify),
                            view);

          if (! gimp_histogram_has_channel (histogram, view->channel))
            gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
        }

      gimp_histogram_view_update_bins (view);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

GimpHistogram *
gimp_histogram_view_get_histogram (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->histogram;
}

void
gimp_histogram_view_set_background (GimpHistogramView *view,
                                    GimpHistogram     *histogram)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));
#if 0
  g_return_if_fail (histogram == NULL ||
                    view->histogram == NULL ||
                    gimp_histogram_n_components (view->histogram) ==
                    gimp_histogram_n_components (histogram));
#endif

  if (view->bg_histogram != histogram)
    {
      if (view->bg_histogram)
        {
          g_signal_handlers_disconnect_by_func (view->bg_histogram,
                                                gimp_histogram_view_notify,
                                                view);
          g_object_unref (view->bg_histogram);
        }

      view->bg_histogram = histogram;

      if (histogram)
        {
          g_object_ref (histogram);

          g_signal_connect (histogram, "notify",
                            G_CALLBACK (gimp_histogram_view_notify),
                            view);

          if (! gimp_histogram_has_channel (histogram, view->channel))
            gimp_histogram_view_set_channel (view, GIMP_HISTOGRAM_VALUE);
        }

      gimp_histogram_view_update_bins (view);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

GimpHistogram *
gimp_histogram_view_get_background (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), NULL);

  return view->bg_histogram;
}

void
gimp_histogram_view_set_channel (GimpHistogramView    *view,
                                 GimpHistogramChannel  channel)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (channel != view->channel)
    g_object_set (view, "histogram-channel", channel, NULL);
}

GimpHistogramChannel
gimp_histogram_view_get_channel (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), 0);

  return view->channel;
}

void
gimp_histogram_view_set_scale (GimpHistogramView  *view,
                               GimpHistogramScale  scale)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (scale != view->scale)
    g_object_set (view, "histogram-scale", scale, NULL);
}

GimpHistogramScale
gimp_histogram_view_get_scale (GimpHistogramView *view)
{
  g_return_val_if_fail (GIMP_IS_HISTOGRAM_VIEW (view), 0);

  return view->scale;
}

void
gimp_histogram_view_set_range (GimpHistogramView *view,
                               gint               start,
                               gint               end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (view->start != MIN (start, end) ||
      view->end   != MAX (start, end))
    {
      view->start = MIN (start, end);
      view->end   = MAX (start, end);

      gtk_widget_queue_draw (GTK_WIDGET (view));

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }
}

void
gimp_histogram_view_get_range (GimpHistogramView *view,
                               gint              *start,
                               gint              *end)
{
  g_return_if_fail (GIMP_IS_HISTOGRAM_VIEW (view));

  if (start) *start = view->start;
  if (end)   *end   = view->end;
}


/*  private functions  */

static void
gimp_histogram_view_notify (GimpHistogram     *histogram,
                            const GParamSpec  *pspec,
                            GimpHistogramView *view)
{
  if (! strcmp (pspec->name, "n-bins"))
    {
      gimp_histogram_view_update_bins (view);
    }
  else
    {
      gtk_widget_queue_draw (GTK_WIDGET (view));
    }
}

static void
gimp_histogram_view_update_bins (GimpHistogramView *view)
{
  gint new_bins = 0;

  if (view->histogram)
    new_bins = gimp_histogram_n_bins (view->histogram);
  else if (view->bg_histogram)
    new_bins = gimp_histogram_n_bins (view->bg_histogram);

  if (new_bins > 0 && new_bins != view->n_bins)
    {
      view->start = MIN (ROUND ((gdouble) view->start *
                                new_bins / view->n_bins),
                         new_bins - 1);
      view->end   = MAX (ROUND ((gdouble) (view->end + 1) *
                                new_bins / view->n_bins) - 1,
                         0);

      view->n_bins = new_bins;

      g_signal_emit (view, histogram_view_signals[RANGE_CHANGED], 0,
                     view->start, view->end);
    }
}
