// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "common/dsp/marker_track.h"
#include "common/dsp/position.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/utils/cairo.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/objects.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/actions.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/timeline.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/arranger_minimap.h"
#include "gui/backend/gtk_widgets/arranger_object.h"
#include "gui/backend/gtk_widgets/arranger_wrapper.h"
#include "gui/backend/gtk_widgets/bot_bar.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/editor_ruler.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/main_notebook.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/midi_arranger.h"
#include "gui/backend/gtk_widgets/midi_modifier_arranger.h"
#include "gui/backend/gtk_widgets/ruler.h"
#include "gui/backend/gtk_widgets/timeline_arranger.h"
#include "gui/backend/gtk_widgets/timeline_panel.h"
#include "gui/backend/gtk_widgets/timeline_ruler.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (RulerWidget, ruler_widget, GTK_TYPE_WIDGET)

#define TYPE(x) RulerWidgetType::x

#define ACTION_IS(x) (self->action == UiOverlayAction::x)

double
ruler_widget_get_zoom_level (RulerWidget * self)
{
  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_val_if_fail (settings, 1.0);

  return settings->hzoom_level_;

  z_return_val_if_reached (1.f);
}

/**
 * Returns the beat interval for drawing vertical
 * lines.
 */
int
ruler_widget_get_beat_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  int beat_divisors[16];
  int num_beat_divisors = 0;
  for (i = 1; i <= beats_per_bar; i++)
    {
      if (beats_per_bar % i == 0)
        beat_divisors[num_beat_divisors++] = i;
    }

  /* decide the raw interval to keep between beats */
  int _beat_interval = MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_beat), 1);

  /* round the interval to the divisors */
  int beat_interval = -1;
  for (i = 0; i < num_beat_divisors; i++)
    {
      if (_beat_interval <= beat_divisors[i])
        {
          if (beat_divisors[i] != beats_per_bar)
            beat_interval = beat_divisors[i];
          break;
        }
    }

  return beat_interval;
}

/**
 * Returns the sixteenth interval for drawing
 * vertical lines.
 */
int
ruler_widget_get_sixteenth_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of sixteenths per beat */
  const auto sixteenths_per_beat = TRANSPORT->sixteenths_per_beat_;
  int        divisors[16];
  int        num_divisors = 0;
  for (i = 1; i <= sixteenths_per_beat; i++)
    {
      if (sixteenths_per_beat % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between sixteenths */
  int _sixteenth_interval =
    MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_sixteenth), 1);

  /* round the interval to the divisors */
  int sixteenth_interval = -1;
  for (i = 0; i < num_divisors; i++)
    {
      if (_sixteenth_interval <= divisors[i])
        {
          if (divisors[i] != sixteenths_per_beat)
            sixteenth_interval = divisors[i];
          break;
        }
    }

  return sixteenth_interval;
}

/**
 * Returns the 10 sec interval.
 */
int
ruler_widget_get_10sec_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of beats per
   * bar */
#define ten_secs_per_min 6
  int ten_sec_divisors[36];
  int num_ten_sec_divisors = 0;
  for (i = 1; i <= ten_secs_per_min; i++)
    {
      if (ten_secs_per_min % i == 0)
        ten_sec_divisors[num_ten_sec_divisors++] = i;
    }

  /* decide the raw interval to keep between
   * 10 secs */
  int _10sec_interval =
    MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_10sec), 1);

  /* round the interval to the divisors */
  int ten_sec_interval = -1;
  for (i = 0; i < num_ten_sec_divisors; i++)
    {
      if (_10sec_interval <= ten_sec_divisors[i])
        {
          if (ten_sec_divisors[i] != ten_secs_per_min)
            ten_sec_interval = ten_sec_divisors[i];
          break;
        }
    }

  return ten_sec_interval;
}

/**
 * Returns the sec interval.
 */
int
ruler_widget_get_sec_interval (RulerWidget * self)
{
  int i;

  /* gather divisors of the number of sixteenths per
   * beat */
#define secs_per_10_sec 10
  int divisors[16];
  int num_divisors = 0;
  for (i = 1; i <= secs_per_10_sec; i++)
    {
      if (secs_per_10_sec % i == 0)
        divisors[num_divisors++] = i;
    }

  /* decide the raw interval to keep between secs */
  int _sec_interval = MAX ((int) (RW_PX_TO_HIDE_BEATS / self->px_per_sec), 1);

  /* round the interval to the divisors */
  int sec_interval = -1;
  for (i = 0; i < num_divisors; i++)
    {
      if (_sec_interval <= divisors[i])
        {
          if (divisors[i] != secs_per_10_sec)
            sec_interval = divisors[i];
          break;
        }
    }

  return sec_interval;
}

/**
 * Draws a region other than the editor one.
 */
static void
draw_other_region (RulerWidget * self, GtkSnapshot * snapshot, Region * region)
{
  /*z_debug (
   * "drawing other region %s", region->name);*/

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  Track * track = region->get_track ();
  auto    color = track->color_;
  color.alpha_ = 0.5;

  int px_start, px_end;
  px_start = ui_pos_to_px_editor (region->pos_, true);
  px_end = ui_pos_to_px_editor (region->end_pos_, true);
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) px_start, 0.f, (float) px_end - (float) px_start,
      (float) height / 4.f);
    z_gtk_snapshot_append_color (snapshot, color, &tmp_r);
  }
}

/**
 * Draws the regions in the editor ruler.
 */
static void
draw_regions (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* get a visible region - the clip editor
   * region is removed temporarily while moving
   * regions so this could be NULL */
  Region * region = CLIP_EDITOR->get_region ();
  if (!region)
    return;

  Track * track = region->get_track ();

  int px_start, px_end;

  /* draw the main region */
  auto color = Color::get_arranger_object_color (
    track->color_, region->is_hovered (), region->is_selected (), false,
    region->get_muted (true));
  px_start = ui_pos_to_px_editor (region->pos_, true);
  px_end = ui_pos_to_px_editor (region->end_pos_, true);
  {
    graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
      (float) px_start, 0.f, (float) px_end - (float) px_start,
      (float) height / 4.f);
    z_gtk_snapshot_append_color (snapshot, color, &tmp_r);
  }

  /* draw its transient if copy-moving TODO */
  if (arranger_object_should_orig_be_visible (*region, nullptr))
    {
      px_start = ui_pos_to_px_editor (region->pos_, 1);
      px_end = ui_pos_to_px_editor (region->end_pos_, 1);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) px_start, 0.f, (float) px_end - (float) px_start,
          (float) height / 4.f);
        z_gtk_snapshot_append_color (snapshot, color, &tmp_r);
      }
    }

  /* draw the other regions */
  std::vector<Region *> other_regions;
  track->get_regions_in_range (other_regions, nullptr, nullptr);
  for (auto &other_region : other_regions)
    {
      if (
        region->id_ == other_region->id_
        || region->id_.type_ != other_region->id_.type_)
        {
          continue;
        }

      draw_other_region (self, snapshot, other_region);
    }
}

static void
get_loop_start_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == RulerWidgetType::Editor)
    {
      Region * region = CLIP_EDITOR->get_region ();
      if (region)
        {
          double start_ticks = region->pos_.ticks_;
          double loop_start_ticks = region->loop_start_pos_.ticks_ + start_ticks;
          Position tmp{ loop_start_ticks };
          rect->x = ui_pos_to_px_editor (tmp, true);
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == RulerWidgetType::Timeline)
    {
      rect->x = ui_pos_to_px_timeline (TRANSPORT->loop_start_pos_, true);
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_start (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_start_rect (self, &dr);
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
  cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_loop_end_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  if (self->type == RulerWidgetType::Editor)
    {
      Region * region = CLIP_EDITOR->get_region ();
      if (region)
        {
          double   start_ticks = region->pos_.ticks_;
          double   loop_end_ticks = region->loop_end_pos_.ticks_ + start_ticks;
          Position tmp{ loop_end_ticks };
          rect->x = ui_pos_to_px_editor (tmp, 1) - RW_RULER_MARKER_SIZE;
        }
      else
        {
          rect->x = 0;
        }
    }
  else if (self->type == RulerWidgetType::Timeline)
    {
      rect->x =
        ui_pos_to_px_timeline (TRANSPORT->loop_end_pos_, 1)
        - RW_RULER_MARKER_SIZE;
    }
  rect->y = 0;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_loop_end (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_loop_end_rect (self, &dr);
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
  cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0, 0.9, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_in_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = ui_pos_to_px_timeline (TRANSPORT->punch_in_pos_, 1);
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_in (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_in_rect (self, &dr);
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
  cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_punch_out_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x =
    ui_pos_to_px_timeline (TRANSPORT->punch_out_pos_, 1) - RW_RULER_MARKER_SIZE;
  rect->y = RW_RULER_MARKER_SIZE;
  rect->width = RW_RULER_MARKER_SIZE;
  rect->height = RW_RULER_MARKER_SIZE;
}

static void
draw_punch_out (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  get_punch_out_rect (self, &dr);
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
  cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      cairo_set_source_rgb (cr, 0.9, 0.1, 0.1);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
get_clip_start_rect (RulerWidget * self, GdkRectangle * rect)
{
  /* TODO these were added to fix unused
   * warnings - check again if valid */
  rect->x = 0;
  rect->y = 0;

  if (self->type == TYPE (Editor))
    {
      Region * region = CLIP_EDITOR->get_region ();
      if (region)
        {
          double start_ticks = region->pos_.ticks_;
          double clip_start_ticks = region->clip_start_pos_.ticks_ + start_ticks;
          Position tmp{ clip_start_ticks };
          rect->x = ui_pos_to_px_editor (tmp, 1);
        }
      else
        {
          rect->x = 0;
        }
      rect->y =
        ((gtk_widget_get_height (GTK_WIDGET (self)) - RW_RULER_MARKER_SIZE)
         - RW_CUE_MARKER_HEIGHT)
        - 1;
    }
  rect->width = RW_CUE_MARKER_WIDTH;
  rect->height = RW_CUE_MARKER_HEIGHT;
}

static void
get_cue_pos_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  rect->y = 0;

  if (self->type == TYPE (Timeline))
    {
      rect->x = ui_pos_to_px_timeline (TRANSPORT->cue_pos_, 1);
      rect->y = RW_RULER_MARKER_SIZE;
    }
  rect->width = RW_CUE_MARKER_WIDTH;
  rect->height = RW_CUE_MARKER_HEIGHT;
}

/**
 * Draws the cue point (or clip start if this is the editor
 * ruler.
 */
static void
draw_cue_point (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  /* draw rect */
  GdkRectangle dr;
  if (self->type == TYPE (Editor))
    {
      get_clip_start_rect (self, &dr);
    }
  else if (self->type == TYPE (Timeline))
    {
      get_cue_pos_rect (self, &dr);
    }
  else
    {
      z_warn_if_reached ();
      return;
    }
  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
  cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

  if (dr.x >= rect->x - dr.width && dr.x <= rect->x + rect->width)
    {
      if (self->type == TYPE (Editor))
        {
          /*cairo_set_source_rgb (cr, 0.2, 0.6, 0.9);*/
          cairo_set_source_rgb (cr, 1, 0.2, 0.2);
        }
      else if (self->type == TYPE (Timeline))
        {
          cairo_set_source_rgb (cr, 0, 0.6, 0.9);
        }
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width), dr.y + dr.height / 2);
      cairo_line_to (cr, dr.x, dr.y + dr.height);
      cairo_fill (cr);
    }

  cairo_destroy (cr);
}

static void
draw_markers (RulerWidget * self, GtkSnapshot * snapshot, int height)
{
  for (auto &m : P_MARKER_TRACK->markers_)
    {
      const float cur_px = (float) ui_pos_to_px_timeline (m->pos_, 1);
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (cur_px, 6.f, 2.f, 12.f);
        z_gtk_snapshot_append_color (snapshot, P_MARKER_TRACK->color_, &tmp_r);
      }

      int textw, texth;
      pango_layout_set_text (self->marker_layout, m->name_.c_str (), -1);
      pango_layout_get_pixel_size (self->marker_layout, &textw, &texth);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (cur_px + 2.f, 6.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      GdkRGBA white = Z_GDK_RGBA_INIT (1, 1, 1, 1.f);
      gtk_snapshot_append_layout (snapshot, self->marker_layout, &white);
      gtk_snapshot_restore (snapshot);
    }
}

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 *
 * @param after_loops Whether to get the playhead px after
 *   loops are applied.
 */
int
ruler_widget_get_playhead_px (RulerWidget * self, bool after_loops)
{
  if (self->type == TYPE (Editor))
    {
      if (after_loops)
        {
          signed_frame_t frames = 0;
          Region *       clip_editor_region = CLIP_EDITOR->get_region ();
          if (!clip_editor_region)
            {
              z_warning ("no clip editor region");
              return ui_pos_to_px_editor (PLAYHEAD, 1);
            }

          Region * r = nullptr;

          if (clip_editor_region->is_automation ())
            {
              auto ar = dynamic_cast<AutomationRegion *> (clip_editor_region);
              AutomationTrack * at = ar->get_automation_track ();
              r = Region::get_at_pos (PLAYHEAD, nullptr, at);
            }
          else
            {
              r = Region::get_at_pos (
                PLAYHEAD, clip_editor_region->get_track (), nullptr);
            }
          Position tmp;
          if (r)
            {
              auto region_local_frames =
                (signed_frame_t) r->timeline_frames_to_local (
                  PLAYHEAD.frames_, true);
              region_local_frames += r->pos_.frames_;
              tmp.from_frames (region_local_frames);
              frames = tmp.frames_;
            }
          else
            {
              frames = PLAYHEAD.frames_;
            }
          Position pos{ frames };
          return ui_pos_to_px_editor (pos, true);
        }
      else /* else if not after loops */
        {
          return ui_pos_to_px_editor (PLAYHEAD, true);
        }
    }
  else if (self->type == TYPE (Timeline))
    {
      return ui_pos_to_px_timeline (PLAYHEAD, 1);
    }
  z_return_val_if_reached (-1);
}

static void
draw_playhead (RulerWidget * self, GtkSnapshot * snapshot, GdkRectangle * rect)
{
  int px = ruler_widget_get_playhead_px (self, true);

  if (
    px >= rect->x - RW_PLAYHEAD_TRIANGLE_WIDTH / 2
    && px <= (rect->x + rect->width) - RW_PLAYHEAD_TRIANGLE_WIDTH / 2)
    {
      self->last_playhead_px = px;

      /* draw rect */
      GdkRectangle dr = { 0, 0, 0, 0 };
      dr.x = px - (RW_PLAYHEAD_TRIANGLE_WIDTH / 2);
      dr.y =
        gtk_widget_get_height (GTK_WIDGET (self)) - RW_PLAYHEAD_TRIANGLE_HEIGHT;
      dr.width = RW_PLAYHEAD_TRIANGLE_WIDTH;
      dr.height = RW_PLAYHEAD_TRIANGLE_HEIGHT;
      graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
        (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
      cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &tmp_r);

      cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
      cairo_set_line_width (cr, 2);
      cairo_move_to (cr, dr.x, dr.y);
      cairo_line_to (cr, (dr.x + dr.width / 2), dr.y + dr.height);
      cairo_line_to (cr, (dr.x + dr.width), dr.y);
      cairo_fill (cr);

      cairo_destroy (cr);
    }
}

/**
 * Draws the grid lines and labels.
 *
 * @param rect Clip rectangle.
 */
static void
draw_lines_and_labels (
  RulerWidget *  self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect)
{
  /* draw lines */
  int    i = 0;
  double curr_px;
  char   text[40];
  int    textw, texth;

  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  GdkRGBA main_color = { 1, 1, 1, 1 };
  GdkRGBA secondary_color = { 0.7f, 0.7f, 0.7f, 0.4f };
  GdkRGBA tertiary_color = { 0.5f, 0.5f, 0.5f, 0.3f };
  GdkRGBA main_text_color = { 0.8f, 0.8f, 0.8f, 1 };
  GdkRGBA secondary_text_color = { 0.5f, 0.5f, 0.5f, 1 };
  GdkRGBA tertiary_text_color = secondary_text_color;

  /* if time display */
  if (
    ENUM_INT_TO_VALUE (
      Transport::Display, g_settings_get_enum (S_UI, "ruler-display"))
    == Transport::Display::Time)
    {
      /* get sec interval */
      int sec_interval = ruler_widget_get_sec_interval (self);

      /* get 10 sec interval */
      int ten_sec_interval = ruler_widget_get_10sec_interval (self);

      /* get the interval for mins */
      int min_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) self->px_per_min, 1.0);

      /* draw mins */
      i = -min_interval;
      while (
        (curr_px = self->px_per_min * (i += min_interval) + SPACE_BEFORE_START)
        < (double) (rect->x + rect->width) + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) curr_px, 0.f, 1.f, (float) height / 3.f);
            gtk_snapshot_append_color (snapshot, &main_color, &tmp_r);
          }

          sprintf (text, "%d", i);
          pango_layout_set_text (self->layout_normal, text, -1);
          pango_layout_get_pixel_size (self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) curr_px + 1.f - (float) textw / 2.f,
              (float) height / 3.f + 2.f);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal, &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw 10secs */
      i = 0;
      if (ten_sec_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_10sec * (i += ten_sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 4.f);
                gtk_snapshot_append_color (snapshot, &secondary_color, &tmp_r);
              }

              if (
                (self->px_per_10sec > RW_PX_TO_HIDE_BEATS * 2)
                && i % ten_secs_per_min != 0)
                {
                  sprintf (
                    text, "%d:%02d", i / ten_secs_per_min,
                    (i % ten_secs_per_min) * 10);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  {
                    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f);
                    gtk_snapshot_translate (snapshot, &tmp_pt);
                  }
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
      /* draw secs */
      i = 0;
      if (sec_interval > 0)
        {
          while (
            (curr_px = self->px_per_sec * (i += sec_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 6.f);
                gtk_snapshot_append_color (snapshot, &tertiary_color, &tmp_r);
              }

              if (
                (self->px_per_sec > RW_PX_TO_HIDE_BEATS * 2)
                && i % secs_per_10_sec != 0)
                {
                  int secs_per_min = 60;
                  sprintf (
                    text, "%d:%02d", i / secs_per_min,
                    ((i / secs_per_10_sec) % ten_secs_per_min) * 10
                      + i % secs_per_10_sec);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  {
                    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f);
                    gtk_snapshot_translate (snapshot, &tmp_pt);
                  }
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
    }
  else /* else if BBT display */
    {
      /* get sixteenth interval */
      int sixteenth_interval = ruler_widget_get_sixteenth_interval (self);

      /* get beat interval */
      int beat_interval = ruler_widget_get_beat_interval (self);

      /* get the interval for bars */
      int bar_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) self->px_per_bar, 1.0);

      /* draw bars */
      i = -bar_interval;
      while (
        (curr_px = self->px_per_bar * (i += bar_interval) + SPACE_BEFORE_START)
        < (double) (rect->x + rect->width) + 20.0)
        {
          if (curr_px + 20.0 < rect->x)
            continue;

          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              (float) curr_px, 0.f, 1.f, (float) height / 3.f);
            gtk_snapshot_append_color (snapshot, &main_color, &tmp_r);
          }

          sprintf (text, "%d", i + 1);
          pango_layout_set_text (self->layout_normal, text, -1);
          pango_layout_get_pixel_size (self->layout_normal, &textw, &texth);
          gtk_snapshot_save (snapshot);
          {
            graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
              (float) curr_px + 1.f - (float) textw / 2.f,
              (float) height / 3.f + 2.f);
            gtk_snapshot_translate (snapshot, &tmp_pt);
          }
          gtk_snapshot_append_layout (
            snapshot, self->layout_normal, &main_text_color);
          gtk_snapshot_restore (snapshot);
        }
      /* draw beats */
      i = 0;
      if (beat_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_beat * (i += beat_interval) + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 4.f);
                gtk_snapshot_append_color (snapshot, &secondary_color, &tmp_r);
              }

              if (
                (self->px_per_beat > RW_PX_TO_HIDE_BEATS * 2)
                && i % beats_per_bar != 0)
                {
                  sprintf (
                    text, "%d.%d", i / beats_per_bar + 1, i % beats_per_bar + 1);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  {
                    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f);
                    gtk_snapshot_translate (snapshot, &tmp_pt);
                  }
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &secondary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
      /* draw sixteenths */
      i = 0;
      if (sixteenth_interval > 0)
        {
          while (
            (curr_px =
               self->px_per_sixteenth * (i += sixteenth_interval)
               + SPACE_BEFORE_START)
            < rect->x + rect->width)
            {
              if (curr_px < rect->x)
                continue;

              {
                graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                  (float) curr_px, 0.f, 1.f, (float) height / 6.f);
                gtk_snapshot_append_color (snapshot, &tertiary_color, &tmp_r);
              }

              const auto sixteenths_per_beat = TRANSPORT->sixteenths_per_beat_;
              if (
                (self->px_per_sixteenth > RW_PX_TO_HIDE_BEATS * 2)
                && i % sixteenths_per_beat != 0)
                {
                  sprintf (
                    text, "%d.%d.%d", i / TRANSPORT->sixteenths_per_bar_ + 1,
                    ((i / sixteenths_per_beat) % beats_per_bar) + 1,
                    i % sixteenths_per_beat + 1);
                  pango_layout_set_text (self->monospace_layout_small, text, -1);
                  pango_layout_get_pixel_size (
                    self->monospace_layout_small, &textw, &texth);
                  gtk_snapshot_save (snapshot);
                  {
                    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
                      (float) curr_px + 1.f - (float) textw / 2.f,
                      (float) height / 4.f + 2.f);
                    gtk_snapshot_translate (snapshot, &tmp_pt);
                  }
                  gtk_snapshot_append_layout (
                    snapshot, self->monospace_layout_small,
                    &tertiary_text_color);
                  gtk_snapshot_restore (snapshot);
                }
            }
        }
    }
}

static void
ruler_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  RulerWidget * self = Z_RULER_WIDGET (widget);

  GdkRectangle visible_rect_gdk;
  ruler_widget_get_visible_rect (self, &visible_rect_gdk);
  graphene_rect_t visible_rect;
  z_gdk_rectangle_to_graphene_rect_t (&visible_rect, &visible_rect_gdk);

  /* engine is run only set after everything is set up so this is a good way to
   * decide if we should  draw or not */
  if (
    !PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->run_.load ()
    || self->px_per_bar < 2.0)
    {
      return;
    }

  self->last_rect = visible_rect;

  /* pretend we're drawing from 0, 0 */
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt =
      Z_GRAPHENE_POINT_INIT (-visible_rect.origin.x, -visible_rect.origin.y);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }

  /* ----- ruler background ------- */

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* if timeline, draw loop background */
  /* FIXME use rect */
  double start_px = 0, end_px = 0;
  if (self->type == TYPE (Timeline))
    {
      start_px = ui_pos_to_px_timeline (TRANSPORT->loop_start_pos_, 1);
      end_px = ui_pos_to_px_timeline (TRANSPORT->loop_end_pos_, 1);
    }
  else if (self->type == TYPE (Editor))
    {
      start_px = ui_pos_to_px_editor (TRANSPORT->loop_start_pos_, 1);
      end_px = ui_pos_to_px_editor (TRANSPORT->loop_end_pos_, 1);
    }

  GdkRGBA color;
  if (TRANSPORT->loop_)
    {
      color.red = 0;
      color.green = 0.9f;
      color.blue = 0.7f;
      color.alpha = 0.25f;
    }
  else
    {
      color.red = 0.5f;
      color.green = 0.5f;
      color.blue = 0.5f;
      color.alpha = 0.25f;
    }
  const int line_width = 2;

  /* if transport loop start is within the screen */
  if (
    start_px + 2.0 > visible_rect_gdk.x
    && start_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop start line */
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) start_px, 0.f, (float) line_width,
          (float) visible_rect.size.height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }
  /* if transport loop end is within the
   * screen */
  if (
    end_px + 2.0 > visible_rect_gdk.x
    && end_px <= visible_rect_gdk.x + visible_rect_gdk.width)
    {
      /* draw the loop end line */
      {
        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
          (float) end_px, 0.f, (float) line_width,
          (float) visible_rect.size.height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }

  /* create gradient for loop area */
  GskColorStop stop1, stop2;
  if (TRANSPORT->loop_)
    {
      stop1.offset = 0;
      stop1.color.red = 0;
      stop1.color.green = 0.9f;
      stop1.color.blue = 0.7f;
      stop1.color.alpha = 0.2f;
      stop2.offset = 1;
      stop2.color.red = 0;
      stop2.color.green = 0.9f;
      stop2.color.blue = 0.7f;
      stop2.color.alpha = 0.1f;
    }
  else
    {
      stop1.offset = 0;
      stop1.color.red = 0.5f;
      stop1.color.green = 0.5f;
      stop1.color.blue = 0.5f;
      stop1.color.alpha = 0.2f;
      stop2.offset = 1;
      stop2.color.red = 0.5f;
      stop2.color.green = 0.5f;
      stop2.color.blue = 0.5f;
      stop2.color.alpha = 0.1f;
    }
  GskColorStop stops[] = { stop1, stop2 };

  graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
    (float) start_px, 0, (float) (end_px - start_px), (float) height);
  graphene_point_t zero_pt = Z_GRAPHENE_POINT_INIT (0, 0);
  graphene_point_t tmp_pt =
    Z_GRAPHENE_POINT_INIT (0.f, ((float) height * 3.f) / 4.f);
  gtk_snapshot_append_linear_gradient (
    snapshot, &tmp_r, &zero_pt, &tmp_pt, stops, G_N_ELEMENTS (stops));

  draw_lines_and_labels (self, snapshot, &visible_rect_gdk);

  /* ----- draw range --------- */

  if (TRANSPORT->has_range_)
    {
      bool range1_first = TRANSPORT->range_1_ <= TRANSPORT->range_2_;

      GdkRectangle dr;
      if (range1_first)
        {
          dr.x = ui_pos_to_px_timeline (TRANSPORT->range_1_, true);
          dr.width = ui_pos_to_px_timeline (TRANSPORT->range_2_, true) - dr.x;
        }
      else
        {
          dr.x = ui_pos_to_px_timeline (TRANSPORT->range_2_, true);
          dr.width = ui_pos_to_px_timeline (TRANSPORT->range_1_, true) - dr.x;
        }
      dr.y = 0;
      dr.height =
        gtk_widget_get_height (GTK_WIDGET (self)) / RW_RANGE_HEIGHT_DIVISOR;

      /* fill */
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.27f);
      {
        graphene_rect_t tmp_r2 = Z_GRAPHENE_RECT_INIT (
          (float) dr.x, (float) dr.y, (float) dr.width, (float) dr.height);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r2);
      }

      /* draw edges */
      tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.7f);
      {
        graphene_rect_t tmp_r3 = Z_GRAPHENE_RECT_INIT (
          (float) dr.x, (float) dr.y, 2.f, (float) dr.height);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r3);
      }
      {
        graphene_rect_t tmp_r4 = Z_GRAPHENE_RECT_INIT (
          (float) dr.x + (float) dr.width, (float) dr.y, 2.f,
          (float) dr.height - (float) dr.y);
        gtk_snapshot_append_color (snapshot, &tmp_color, &tmp_r4);
      }
    }

  /* ----- draw regions --------- */

  if (self->type == TYPE (Editor))
    {
      draw_regions (self, snapshot, &visible_rect_gdk);
    }

  /* ------ draw markers ------- */

  draw_cue_point (self, snapshot, &visible_rect_gdk);
  draw_loop_start (self, snapshot, &visible_rect_gdk);
  draw_loop_end (self, snapshot, &visible_rect_gdk);

  if (self->type == TYPE (Timeline) && TRANSPORT->punch_mode_)
    {
      draw_punch_in (self, snapshot, &visible_rect_gdk);
      draw_punch_out (self, snapshot, &visible_rect_gdk);
    }

  if (self->type == TYPE (Editor))
    {
      draw_markers (self, snapshot, height);
    }

  /* --------- draw playhead ---------- */

  draw_playhead (self, snapshot, &visible_rect_gdk);

  gtk_snapshot_restore (snapshot);
}

#undef beats_per_bar
#undef sixteenths_per_beat

static int
is_loop_start_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_loop_start_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_loop_end_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_loop_end_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_punch_in_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_punch_in_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static int
is_punch_out_hit (RulerWidget * self, double x, double y)
{
  GdkRectangle rect;
  get_punch_out_rect (self, &rect);

  return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
         && y <= rect.y + rect.height;
}

static bool
is_clip_start_hit (RulerWidget * self, double x, double y)
{
  if (self->type == TYPE (Editor))
    {
      GdkRectangle rect;
      get_clip_start_rect (self, &rect);

      return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
             && y <= rect.y + rect.height;
    }
  else
    return false;
}

static void
get_range_rect (RulerWidget * self, RulerWidgetRangeType type, GdkRectangle * rect)
{
  z_return_if_fail (self->type == TYPE (Timeline));

  auto [start, end] = TRANSPORT->get_range_positions ();
  Position tmp = type == RulerWidgetRangeType::Start ? start : end;
  rect->x = ui_pos_to_px_timeline (tmp, true);
  if (type == RulerWidgetRangeType::End)
    {
      rect->x -= RW_CUE_MARKER_WIDTH;
    }
  rect->y = 0;
  if (type == RulerWidgetRangeType::Full)
    {
      double px = ui_pos_to_px_timeline (end, true);
      rect->width = (int) px;
    }
  else
    {
      rect->width = RW_CUE_MARKER_WIDTH;
    }
  rect->height =
    gtk_widget_get_height (GTK_WIDGET (self)) / RW_RANGE_HEIGHT_DIVISOR;
}

static void
get_loop_range_rect (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  GdkRectangle *       rect)
{
  auto [start, end] = TRANSPORT->get_loop_range_positions ();
  Position tmp = type == RulerWidgetRangeType::Start ? start : end;
  rect->x = ui_pos_to_px_timeline (tmp, true);
  if (type == RulerWidgetRangeType::End)
    {
      rect->x -= RW_CUE_MARKER_WIDTH;
    }
  rect->y = 0;
  if (type == RulerWidgetRangeType::Full)
    {
      double px = ui_pos_to_px_timeline (end, true);
      rect->width = (int) px;
    }
  else
    {
      rect->width = RW_CUE_MARKER_WIDTH;
    }
  rect->height =
    gtk_widget_get_height (GTK_WIDGET (self)) / RW_RANGE_HEIGHT_DIVISOR;
}

bool
ruler_widget_is_range_hit (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  double               x,
  double               y)
{
  if (self->type == TYPE (Timeline) && TRANSPORT->has_range_)
    {
      GdkRectangle rect;
      memset (&rect, 0, sizeof (GdkRectangle));
      get_range_rect (self, type, &rect);

      return x >= rect.x && x <= rect.x + rect.width && y >= rect.y
             && y <= rect.y + rect.height;
    }
  else
    {
      return false;
    }
}

bool
ruler_widget_is_loop_range_hit (
  RulerWidget *        self,
  RulerWidgetRangeType type,
  double               x,
  double               y)
{
  GdkRectangle rect;
  memset (&rect, 0, sizeof (GdkRectangle));
  get_loop_range_rect (self, type, &rect);

  return x >= rect.x && x <= rect.x + rect.width;
}

static gboolean
on_click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  RulerWidget *     self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_SHIFT_MASK)
    self->shift_held = 1;
  if (state & GDK_ALT_MASK)
    self->alt_held = true;
  if (state & GDK_CONTROL_MASK)
    self->ctrl_held = true;

  if (n_press == 2)
    {
      if (self->type == TYPE (Timeline))
        {
          Position pos = ui_px_to_pos_timeline (x, true);
          if (!self->shift_held && SNAP_GRID_TIMELINE->any_snap ())
            {
              pos.snap (&pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
            }
          TRANSPORT->cue_pos_ = pos;
        }
      if (self->type == TYPE (Editor))
        {
        }
    }

  return FALSE;
}

static gboolean
on_right_click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  RulerWidget *     self)
{
  /* right click */
  if (n_press == 1)
    {
      GMenu * menu = g_menu_new ();

      GMenu * section = g_menu_new ();
      g_menu_append (section, _ ("BBT"), "ruler.ruler-display::bbt");
      g_menu_append (section, _ ("Time"), "ruler.ruler-display::time");
      g_menu_append_section (menu, _ ("Display"), G_MENU_MODEL (section));

      z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
    }

  return FALSE;
}

/**
 * Sets the appropriate cursor based on the current
 * drag/hover/action status.
 */
static void
set_cursor (RulerWidget * self)
{
  if (self->action == UiOverlayAction::None)
    {
      if (self->hovering)
        {
          double x = self->hover_x;
          double y = self->hover_y;
          bool   punch_in_hit = is_punch_in_hit (self, x, y);
          bool   punch_out_hit = is_punch_out_hit (self, x, y);
          bool   loop_start_hit = is_loop_start_hit (self, x, y);
          bool   loop_end_hit = is_loop_end_hit (self, x, y);
          bool   clip_start_hit = is_clip_start_hit (self, x, y);
          bool   range_start_hit =
            ruler_widget_is_range_hit (self, RulerWidgetRangeType::Start, x, y);
          bool range_end_hit =
            ruler_widget_is_range_hit (self, RulerWidgetRangeType::End, x, y);

          int height = gtk_widget_get_height (GTK_WIDGET (self));
          if (self->alt_held)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "all-scroll");
            }
          else if (
            punch_in_hit || loop_start_hit || clip_start_hit || range_start_hit)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "w-resize");
            }
          else if (punch_out_hit || loop_end_hit || range_end_hit)
            {
              ui_set_cursor_from_name (GTK_WIDGET (self), "e-resize");
            }
          /* if lower 3/4ths */
          else if (y > (height * 1) / 4)
            {
              /* set cursor to normal */
              /*z_debug ("lower 3/4ths - setting default");*/
              ui_set_cursor_from_name (GTK_WIDGET (self), "default");
              if (
                self->ctrl_held
                && ruler_widget_is_loop_range_hit (
                  self, RulerWidgetRangeType::Full, x, y))
                {
                  /* set cursor to movable */
                  ui_set_hand_cursor (self);
                }
            }
          else /* upper 1/4th */
            {
              if (ruler_widget_is_range_hit (
                    self, RulerWidgetRangeType::Full, x, y))
                {
                  /* set cursor to movable */
                  ui_set_hand_cursor (self);
                }
              else
                {
                  /* set cursor to range selection */
                  ui_set_time_select_cursor (self);
                }
            }
        }
      else
        {
          /*z_debug ("no hover - setting default");*/
          ui_set_cursor_from_name (GTK_WIDGET (self), "default");
        }
    }
  else
    {
      switch (self->action)
        {
        case UiOverlayAction::StartingPanning:
        case UiOverlayAction::Panning:
          ui_set_cursor_from_name (GTK_WIDGET (self), "all-scroll");
          break;
        case UiOverlayAction::STARTING_MOVING:
        case UiOverlayAction::MOVING:
          ui_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
          break;
        default:
          /*z_debug ("no known action - setting default");*/
          ui_set_cursor_from_name (GTK_WIDGET (self), "default");
          break;
        }
    }
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  RulerWidget *    self)
{
  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_if_fail (settings);
  start_x += settings->scroll_start_x_;
  self->start_x = start_x;
  self->start_y = start_y;

  guint drag_start_btn =
    gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  if (self->type == TYPE (Timeline))
    {
      self->range1_first = TRANSPORT->range_1_ <= TRANSPORT->range_2_;
    }

  int height = gtk_widget_get_height (GTK_WIDGET (self));

  int punch_in_hit = is_punch_in_hit (self, start_x, start_y);
  int punch_out_hit = is_punch_out_hit (self, start_x, start_y);
  int loop_start_hit = is_loop_start_hit (self, start_x, start_y);
  int loop_end_hit = is_loop_end_hit (self, start_x, start_y);
  int clip_start_hit = is_clip_start_hit (self, start_x, start_y);

  Region * region = CLIP_EDITOR->get_region ();

  /* if alt held down, start panning */
  if (self->alt_held || drag_start_btn == GDK_BUTTON_MIDDLE)
    {
      self->action = UiOverlayAction::StartingPanning;
    }
  /* else if one of the markers hit */
  else if (punch_in_hit)
    {
      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::PunchIn;
    }
  else if (punch_out_hit)
    {
      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::PunchOut;
    }
  else if (loop_start_hit)
    {
      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::LoopStart;
      if (self->type == TYPE (Editor))
        {
          z_return_if_fail (region);
          self->drag_start_pos = region->loop_start_pos_;
        }
      else
        {
          self->drag_start_pos = TRANSPORT->loop_start_pos_;
        }
    }
  else if (loop_end_hit)
    {
      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::LoopEnd;
      if (self->type == TYPE (Editor))
        {
          z_return_if_fail (region);
          self->drag_start_pos = region->loop_end_pos_;
        }
      else
        {
          self->drag_start_pos = TRANSPORT->loop_end_pos_;
        }
    }
  else if (clip_start_hit)
    {
      self->action = UiOverlayAction::STARTING_MOVING;
      self->target = RWTarget::ClipStart;
      if (self->type == TYPE (Editor))
        {
          z_return_if_fail (region);
          self->drag_start_pos = region->clip_start_pos_;
        }
    }
  else
    {
      if (self->type == TYPE (Timeline))
        {
          timeline_ruler_on_drag_begin_no_marker_hit (
            self, start_x, start_y, height);
        }
      else if (self->type == TYPE (Editor))
        {
          editor_ruler_on_drag_begin_no_marker_hit (self, start_x, start_y);
        }
    }

  self->last_offset_x = 0;
  self->last_offset_y = 0;
  self->dragging = 1;
  self->vertical_panning_started = false;

  set_cursor (self);
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  RulerWidget *    self)
{
  self->start_x = 0;
  self->start_y = 0;
  self->shift_held = 0;
  self->dragging = 0;

  if (self->type == TYPE (Timeline))
    timeline_ruler_on_drag_end (self);
  else if (self->type == TYPE (Editor))
    editor_ruler_on_drag_end (self);

  self->action = UiOverlayAction::None;

  set_cursor (self);
}

/**
 * Taken from ArrangerWidget. Same logic. FIXME make this code common somewhere.
 */
static void
auto_scroll (RulerWidget * self)
{
  /* figure out if we should scroll */
  bool scroll_h = false;
  switch (self->action)
    {
    case UiOverlayAction::MOVING:
      scroll_h = true;
      break;
    default:
      break;
    }

  if (!scroll_h)
    return;

  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_if_fail (settings);
  int h_scroll_speed = 20;
  int border_distance = 5;
  int scroll_width = gtk_widget_get_width (GTK_WIDGET (self));
  int h_delta = 0;
  int adj_x = settings->scroll_start_x_;
  int x = (int) self->hover_x;
  if (x + border_distance >= adj_x + scroll_width)
    {
      h_delta = h_scroll_speed;
    }
  else if (x - border_distance <= adj_x)
    {
      h_delta = -h_scroll_speed;
    }

  if (!scroll_h)
    h_delta = 0;

  if (settings->scroll_start_x_ + h_delta < 0)
    {
      h_delta -= settings->scroll_start_x_ + h_delta;
    }
  settings->append_scroll (h_delta, 0, F_VALIDATE);
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  RulerWidget *              self)
{
  GdkEvent * event = gtk_event_controller_get_current_event (
    GTK_EVENT_CONTROLLER (motion_controller));
  GdkEventType    event_type = gdk_event_get_event_type (event);
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (motion_controller));

  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_if_fail (settings);
  x += settings->scroll_start_x_;
  self->hover_x = x;
  self->hover_y = y;
  self->hovering = event_type != GDK_LEAVE_NOTIFY;

  self->shift_held = state & GDK_SHIFT_MASK;
  self->alt_held = state & GDK_ALT_MASK;
  self->ctrl_held = state & GDK_CONTROL_MASK;

  /* drag-update didn't work so do the drag-update here */
  if (self->dragging && event_type != GDK_LEAVE_NOTIFY)
    {
      if (ACTION_IS (STARTING_MOVING))
        {
          self->action = UiOverlayAction::MOVING;
        }
      else if (ACTION_IS (StartingPanning))
        {
          self->action = UiOverlayAction::Panning;
        }

      /* panning is common */
      double total_offset_x = x - self->start_x;
      double offset_x = total_offset_x - self->last_offset_x;
      double total_offset_y = y - self->start_y;
      double offset_y = total_offset_y - self->last_offset_y;
      if (self->action == UiOverlayAction::Panning)
        {
          if (!math_doubles_equal_epsilon (offset_x, 0.0, 0.1))
            {
              /* pan horizontally */
              settings->append_scroll ((int) -offset_x, 0, F_VALIDATE);

              /* these are also affected */
              self->last_offset_x = MAX (0, self->last_offset_x - offset_x);
              self->hover_x = MAX (0, self->hover_x - offset_x);
              self->start_x = MAX (0, self->start_x - offset_x);
            }
          int drag_threshold =
            zrythm_app->default_settings_->property_gtk_dnd_drag_threshold ()
              .get_value ();
          if (
            !math_doubles_equal_epsilon (offset_y, 0.0, 0.1)
            && (fabs (total_offset_y) >= drag_threshold || self->vertical_panning_started))
            {
              /* get position of cursor */
              Position cursor_pos;
              ruler_widget_px_to_pos (
                self, self->hover_x, &cursor_pos, F_PADDING);

              /* get px diff so we can calculate the new adjustment later */
              double diff = self->hover_x - (double) settings->scroll_start_x_;

              double scroll_multiplier = 1.0 - 0.02 * offset_y;
              ruler_widget_set_zoom_level (
                self, ruler_widget_get_zoom_level (self) * scroll_multiplier);

              int new_x = ruler_widget_pos_to_px (self, &cursor_pos, F_PADDING);

              settings->set_scroll_start_x (new_x - (int) diff, F_VALIDATE);

              /* also update hover x since we're using it here */
              self->hover_x = new_x;
              self->start_x = self->hover_x - total_offset_x;

              self->vertical_panning_started = true;
            }
        }
      else if (self->type == TYPE (Timeline))
        {
          timeline_ruler_on_drag_update (self, total_offset_x, total_offset_y);
        }
      else if (self->type == TYPE (Editor))
        {
          editor_ruler_on_drag_update (self, total_offset_x, total_offset_y);
        }

      auto_scroll (self);

      self->last_offset_x = total_offset_x;
      self->last_offset_y = total_offset_y;

      set_cursor (self);

      return;
    }

  set_cursor (self);
}

static void
on_leave (GtkEventControllerMotion * motion_controller, RulerWidget * self)
{
  self->hovering = false;
}

void
ruler_widget_refresh (RulerWidget * self)
{
  /*adjust for zoom level*/
  self->px_per_tick = DEFAULT_PX_PER_TICK * ruler_widget_get_zoom_level (self);
  self->px_per_sixteenth = self->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  self->px_per_beat = self->px_per_tick * TRANSPORT->ticks_per_beat_;
  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  self->px_per_bar = self->px_per_beat * beats_per_bar;

  Position pos;
  pos.from_seconds (1);
  self->px_per_min = 60.0 * pos.ticks_ * self->px_per_tick;
  self->px_per_10sec = 10.0 * pos.ticks_ * self->px_per_tick;
  self->px_per_sec = pos.ticks_ * self->px_per_tick;
  self->px_per_100ms = 0.1 * pos.ticks_ * self->px_per_tick;

  pos.set_to_bar (TRANSPORT->total_bars_ + 1);
  double prev_total_px = self->total_px;
  self->total_px = self->px_per_tick * (double) pos.ticks_;

  /* if size changed */
  if (!math_doubles_equal_epsilon (prev_total_px, self->total_px, 0.1))
    {
      EVENTS_PUSH (EventType::ET_RULER_SIZE_CHANGED, self);
    }
}

/**
 * Gets the pointer to the EditorSettings associated with the
 * arranger this ruler is for.
 */
EditorSettings *
ruler_widget_get_editor_settings (RulerWidget * self)
{
  if (self->type == RulerWidgetType::Timeline)
    {
      return PRJ_TIMELINE.get ();
    }
  else if (self->type == RulerWidgetType::Editor)
    {
      ArrangerWidget * arr =
        clip_editor_inner_widget_get_visible_arranger (MW_CLIP_EDITOR_INNER);
      auto settings = arranger_widget_get_editor_settings (arr);
      return std::visit (
        [&] (auto &&s) -> EditorSettings * { return s; }, settings);
    }
  z_return_val_if_reached (nullptr);
}

/**
 * Fills in the visible rectangle.
 */
void
ruler_widget_get_visible_rect (RulerWidget * self, GdkRectangle * rect)
{
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
  const EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_if_fail (settings);
  rect->x = settings->scroll_start_x_;
  rect->y = 0;
}

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * @return Whether the zoom level was set.
 */
bool
ruler_widget_set_zoom_level (RulerWidget * self, double zoom_level)
{
  if (zoom_level > MAX_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled ("zoom-in", false);
    }
  else
    {
      actions_set_app_action_enabled ("zoom-in", true);
    }
  if (zoom_level < MIN_ZOOM_LEVEL)
    {
      actions_set_app_action_enabled ("zoom-out", false);
    }
  else
    {
      actions_set_app_action_enabled ("zoom-out", true);
    }

  int update = zoom_level >= MIN_ZOOM_LEVEL && zoom_level <= MAX_ZOOM_LEVEL;

  if (update)
    {
      EditorSettings * settings = ruler_widget_get_editor_settings (self);
      z_return_val_if_fail (settings, false);
      settings->hzoom_level_ = zoom_level;
      ruler_widget_refresh (self);
      return true;
    }
  else
    {
      return false;
    }
}

void
ruler_widget_px_to_pos (
  RulerWidget * self,
  double        px,
  Position *    pos,
  bool          has_padding)
{
  if (self->type == TYPE (Timeline))
    {
      *pos = ui_px_to_pos_timeline (px, has_padding);
    }
  else
    {
      *pos = ui_px_to_pos_editor (px, has_padding);
    }
}

int
ruler_widget_pos_to_px (RulerWidget * self, Position * pos, int use_padding)
{
  if (self->type == TYPE (Timeline))
    {
      return ui_pos_to_px_timeline (*pos, use_padding);
    }
  else
    {
      return ui_pos_to_px_editor (*pos, use_padding);
    }
}

void
ruler_widget_handle_horizontal_zoom (RulerWidget * self, double * x_pos, double dy)
{
  /* get current adjustment so we can get the difference from the cursor */
  EditorSettings * settings = ruler_widget_get_editor_settings (self);
  z_return_if_fail (settings);

  /* get position of cursor */
  Position cursor_pos;
  ruler_widget_px_to_pos (self, *x_pos, &cursor_pos, F_PADDING);
  /*position_print (&cursor_pos);*/

  /* get px diff so we can calculate the new adjustment later */
  double diff = *x_pos - (double) settings->scroll_start_x_;

  double scroll_multiplier = (dy > 0) ? (1.0 / 1.3) : 1.3;
  ruler_widget_set_zoom_level (
    self, ruler_widget_get_zoom_level (self) * scroll_multiplier);

  int new_x = ruler_widget_pos_to_px (self, &cursor_pos, F_PADDING);

  settings->set_scroll_start_x (new_x - (int) diff, F_VALIDATE);

  /* refresh relevant widgets */
  if (self->type == TYPE (Timeline))
    {
      arranger_minimap_widget_refresh (MW_TIMELINE_MINIMAP);
    }

  /* also update hover x */
  *x_pos = new_x;
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  RulerWidget * self = Z_RULER_WIDGET (user_data);

  GdkModifierType modifier_type = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  z_debug (
    "scrolled to %.1f (d %d), %.1f (d %d)", self->hover_x, (int) dx,
    self->hover_y, (int) dy);

  bool ctrl_held = modifier_type & GDK_CONTROL_MASK;
  bool shift_held = modifier_type & GDK_SHIFT_MASK;
  if (ctrl_held)
    {
      if (!shift_held)
        {
          ruler_widget_handle_horizontal_zoom (self, &self->hover_x, dy);
        }
    }
  else /* else if not ctrl held */
    {
      /* scroll normally */
      const bool horizontal_scroll = ((int) dx) != 0;
      if ((modifier_type & GDK_SHIFT_MASK) || horizontal_scroll)
        {
          const int        scroll_amt = RW_SCROLL_SPEED;
          int              scroll_x = horizontal_scroll ? (int) dx : (int) dy;
          EditorSettings * settings = ruler_widget_get_editor_settings (self);
          settings->append_scroll (scroll_x * scroll_amt, 0, F_VALIDATE);
        }
    }

  return true;
}

static guint
ruler_tick_cb (
  GtkWidget *     widget,
  GtkTickCallback callback,
  gpointer        user_data,
  GDestroyNotify  notify)
{
  gtk_widget_queue_draw (widget);

  return 5;
}

static void
dispose (RulerWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  object_free_w_func_and_null (g_object_unref, self->layout_normal);
  object_free_w_func_and_null (g_object_unref, self->monospace_layout_small);
  object_free_w_func_and_null (g_object_unref, self->marker_layout);

  G_OBJECT_CLASS (ruler_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
ruler_finalize (GObject * object)
{
  RulerWidget * self = Z_RULER_WIDGET (object);

  std::destroy_at (&self->range1_start_pos);
  std::destroy_at (&self->range2_start_pos);
  std::destroy_at (&self->last_set_pos);
  std::destroy_at (&self->drag_start_pos);

  G_OBJECT_CLASS (ruler_widget_parent_class)->finalize (object);
}

static void
ruler_widget_init (RulerWidget * self)
{
  std::construct_at (&self->range1_start_pos);
  std::construct_at (&self->range2_start_pos);
  std::construct_at (&self->last_set_pos);
  std::construct_at (&self->drag_start_pos);

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_hexpand (GTK_WIDGET (self), true);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  /* allow all buttons for drag */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->drag), 0);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (on_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));

  GtkGestureClick * right_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed", G_CALLBACK (on_right_click_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_mp));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  self->layout_normal =
    gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
  PangoFontDescription * desc =
    pango_font_description_from_string ("Monospace 11");
  pango_layout_set_font_description (self->layout_normal, desc);
  pango_font_description_free (desc);

  self->monospace_layout_small =
    gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
  desc = pango_font_description_from_string ("Monospace 6");
  pango_layout_set_font_description (self->monospace_layout_small, desc);
  pango_font_description_free (desc);

  self->marker_layout =
    gtk_widget_create_pango_layout (GTK_WIDGET (self), nullptr);
  desc = pango_font_description_from_string ("7");
  pango_layout_set_font_description (self->marker_layout, desc);
  pango_font_description_free (desc);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) ruler_tick_cb, self, nullptr);

  /* add action group for right click menu */
  GSimpleActionGroup * action_group = g_simple_action_group_new ();
  GAction * display_action = g_settings_create_action (S_UI, "ruler-display");
  g_action_map_add_action (G_ACTION_MAP (action_group), display_action);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self), "ruler", G_ACTION_GROUP (action_group));

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

static void
ruler_widget_class_init (RulerWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = ruler_snapshot;
  gtk_widget_class_set_css_name (wklass, "ruler");

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
  oklass->finalize = ruler_finalize;
}
