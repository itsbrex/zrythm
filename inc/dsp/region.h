// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * A region in the timeline.
 */
#ifndef __AUDIO_REGION_H__
#define __AUDIO_REGION_H__

#include "dsp/automation_point.h"
#include "dsp/chord_object.h"
#include "dsp/midi_note.h"
#include "dsp/position.h"
#include "dsp/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

#include <glib/gi18n.h>

typedef struct Channel         Channel;
typedef struct Track           Track;
typedef struct MidiNote        MidiNote;
typedef struct TrackLane       TrackLane;
typedef struct RegionLinkGroup RegionLinkGroup;
typedef struct Stretcher       Stretcher;
typedef struct AudioClip       AudioClip;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define REGION_MAGIC 93075327
#define IS_REGION(x) (((Region *) x)->magic == REGION_MAGIC)
#define IS_REGION_AND_NONNULL(x) (x && IS_REGION (x))

#define REGION_PRINTF_FILENAME "%s_%s.mid"

#define region_is_selected(r) arranger_object_is_selected ((ArrangerObject *) r)

/**
 * Musical mode setting for audio regions.
 */
enum class RegionMusicalMode
{
  /** Inherit from global musical mode setting. */
  REGION_MUSICAL_MODE_INHERIT,
  /** Musical mode off - don't auto-stretch when
   * BPM changes. */
  REGION_MUSICAL_MODE_OFF,
  /** Musical mode on - auto-stretch when BPM
   * changes. */
  REGION_MUSICAL_MODE_ON,
};

const char *
region_musical_mode_to_str (RegionMusicalMode mode);

/**
 * A region (clip) is an object on the timeline that contains
 * either MidiNote's or AudioClip's.
 *
 * It is uniquely identified using its name, so it must be
 * unique throughout the Project.
 *
 * @extends ArrangerObject
 */
typedef struct Region
{
  /**
   * Base struct.
   */
  ArrangerObject base;

  /** Unique ID. */
  RegionIdentifier id;

  /** Name to be shown on the widget. */
  char * name;

  /** Escaped name for drawing. */
  char * escaped_name;

  /**
   * Region color independent of track.
   */
  GdkRGBA color;

  /**
   * Whether to use the custom color.
   *
   * If false, the track color will be used.
   */
  bool use_color;

  /* ==== MIDI REGION ==== */

  /**
   * MIDI notes.
   */
  MidiNote ** midi_notes;
  int         num_midi_notes;
  size_t      midi_notes_size;

  /**
   * Unended notes started in recording with
   * MIDI NOTE ON
   * signal but haven't received a NOTE OFF yet.
   *
   * This is also used temporarily when reading
   * from MIDI files.
   *
   * FIXME allocate.
   *
   * @note These are present in
   *   \ref Region.midi_notes and must not be
   *   free'd separately.
   */
  MidiNote * unended_notes[12000];
  int        num_unended_notes;

  /* ==== MIDI REGION END ==== */

  /* ==== AUDIO REGION ==== */

  /** Audio pool ID of the associated audio file, mostly used during
   * serialization. */
  int pool_id;

  /**
   * Whether currently running the stretching algorithm.
   *
   * If this is true, region drawing will be deferred.
   */
  bool stretching;

  /**
   * The length before stretching, in ticks.
   */
  double before_length;

  /** Used during arranger UI overlay actions. */
  double stretch_ratio;

  /**
   * Whether to read the clip from the pool (used in most cases).
   */
  bool read_from_pool;

  /** Gain to apply to the audio (amplitude 0.0-2.0). */
  float gain;

  /**
   * Clip to read frames from, if not from the pool.
   */
  AudioClip * clip;

#if 0
  /**
   * Frames to actually use, interleaved.
   *
   * Properties such as \ref AudioClip.channels can
   * be fetched from the AudioClip.
   */
  sample_t *        frames;
  size_t            num_frames;

  /**
   * Per-channel frames for convenience.
   */
  sample_t *        ch_frames[16];
#endif

  /** Musical mode setting. */
  RegionMusicalMode musical_mode;

  /** Array of split points. */
  Position * split_points;
  int        num_split_points;
  size_t     split_points_size;

  /* ==== AUDIO REGION END ==== */

  /* ==== AUTOMATION REGION ==== */

  /**
   * The automation points this region contains.
   *
   * Could also be used in audio regions for volume
   * automation.
   *
   * Must always stay sorted by position.
   */
  AutomationPoint ** aps;
  int                num_aps;
  size_t             aps_size;

  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap;

  /* ==== AUTOMATION REGION END ==== */

  /* ==== CHORD REGION ==== */

  /** ChordObject's in this Region. */
  ChordObject ** chord_objects;
  int            num_chord_objects;
  size_t         chord_objects_size;

  /* ==== CHORD REGION END ==== */

  /**
   * Set to ON during bouncing if this
   * region should be included.
   *
   * Only relevant for audio and midi regions.
   */
  int bounce;

  /* --- drawing caches --- */

  /* New region drawing needs to be cached in the
   * following situations:
   *
   * 1. when hidden part of the region is revealed
   *   (on x axis).
   *   TODO max 140% of the region should be
   *   cached (20% before and 20% after if before/
   *   after is not fully visible)
   * 2. when full rect (x/width) changes
   * 3. when a region marker is moved
   * 4. when the clip actually changes (use
   *   last-change timestamp on the clip or region)
   * 5. when fades change
   * 6. when region height changes (track/lane)
   */

  /** Cache layout for drawing the name. */
  PangoLayout * layout;

  /** Cache layout for drawing the chord names
   * inside the region. */
  PangoLayout * chords_layout;

  /* these are used for caching */
  GdkRectangle last_main_full_rect;

  /** Last main draw rect. */
  GdkRectangle last_main_draw_rect;

  /* these are used for caching */
  GdkRectangle last_lane_full_rect;

  /** Last lane draw rect. */
  GdkRectangle last_lane_draw_rect;

  /** Last timestamp the audio clip or its contents
   * changed. */
  gint64 last_clip_change;

  /** Last timestamp the region was cached. */
  gint64 last_cache_time;

  /** Last known marker positions (only positions
   * are used). */
  ArrangerObject last_positions_obj;

  /* --- drawing caches end --- */

  TrackLane * owner_lane = nullptr;

  int magic;
} Region;

/**
 * Returns if the given Region type can have fades.
 */
#define region_type_can_fade(rtype) (rtype == RegionType::REGION_TYPE_AUDIO)

/**
 * Only to be used by implementing structs.
 */
void
region_init (
  Region *         region,
  const Position * start_pos,
  const Position * end_pos,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane);

/**
 * Looks for the Region matching the identifier.
 */
HOT NONNULL Region *
region_find (const RegionIdentifier * const id);

#if 0
static inline void
region_set_track_name_hash (
  Region *    self,
  unsigned int name_hash)
{
  self->id.track_name_hash = name_hash;
}
#endif

NONNULL void
region_print_to_str (const Region * self, char * buf, const size_t buf_size);

/**
 * Print region info for debugging.
 *
 * @public @memberof Region
 */
NONNULL void
region_print (const Region * region);

/**
 * Get lane.
 *
 * @public @memberof Region
 */
TrackLane *
region_get_lane (const Region * region);

/**
 * Returns the region's link group.
 *
 * @public @memberof Region
 */
RegionLinkGroup *
region_get_link_group (Region * self);

/**
 * Sets the link group to the region.
 *
 * @param group_idx If -1, the region will be
 *   removed from its current link group, if any.
 *
 * @public @memberof Region
 */
void
region_set_link_group (Region * region, int group_idx, bool update_identifier);

NONNULL bool
region_has_link_group (Region * region);

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 *
 * @public @memberof Region
 */
MidiNote *
region_find_midi_note (Region * r, MidiNote * _mn);

/**
 * Converts frames on the timeline (global)
 * to local frames (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_frames Timeline position in
 *   frames.
 *
 * @return The local frames.
 *
 * @public @memberof Region
 */
NONNULL HOT signed_frame_t
region_timeline_frames_to_local (
  const Region * const self,
  const signed_frame_t timeline_frames,
  const bool           normalize);

/**
 * Returns the number of frames until the next loop end point or the end of the
 * region.
 *
 * @param[in] timeline_frames Global frames at start.
 * @param[out] ret_frames Return frames.
 * @param[out] is_loop Whether the return frames are for a loop (if false,
 *   the return frames are for the region's end).
 *
 * @public @memberof Region
 */
NONNULL void
region_get_frames_till_next_loop_or_end (
  const Region * const self,
  const signed_frame_t timeline_frames,
  signed_frame_t *     ret_frames,
  bool *               is_loop);

/**
 * Sets the track lane.
 *
 * @public @memberof Region
 */
NONNULL void
region_set_lane (Region * self, TrackLane * lane);

/**
 * Generates a name for the Region, either using
 * the given AutomationTrack or Track, or appending
 * to the given base name.
 *
 * @public @memberof Region
 */
void
region_gen_name (
  Region *          region,
  const char *      base_name,
  AutomationTrack * at,
  Track *           track);

/**
 * Stretch the region's contents.
 *
 * This should be called right after changing the
 * region's size.
 *
 * @param ratio The ratio to stretch by.
 *
 * @return Whether successful.
 *
 * @public @memberof Region
 */
NONNULL WARN_UNUSED_RESULT bool
region_stretch (Region * self, double ratio, GError ** error);

/**
 * To be called every time the identifier changes
 * to update the region's children.
 *
 * @public @memberof Region
 */
NONNULL void
region_update_identifier (Region * self);

/**
 * Updates all other regions in the region link
 * group, if any.
 *
 * @public @memberof Region
 */
NONNULL void
region_update_link_group (Region * self);

/**
 * Moves the Region to the given Track, maintaining the
 * selection status of the Region.
 *
 * Assumes that the Region is already in a TrackLane or
 * AutomationTrack.
 *
 * @param lane_or_at_index If MIDI or audio, lane position.
 *   If automation, automation track index in the automation
 *   tracklist. If -1, the track lane or automation track
 *   index will be inferred from the region.
 * @param index If MIDI or audio, index in lane in the new
 *   track to insert the region to, or -1 to append. If
 *   automation, index in the automation track.
 *
 * @public @memberof Region
 */
void
region_move_to_track (
  Region * region,
  Track *  track,
  int      lane_or_at_index,
  int      index);

/**
 * Returns if the given Region type can exist
 * in TrackLane's.
 */
CONST
static inline int
region_type_has_lane (const RegionType type)
{
  return type == RegionType::REGION_TYPE_MIDI
         || type == RegionType::REGION_TYPE_AUDIO;
}

/**
 * Sets the automation track.
 *
 * @public @memberof Region
 */
void
region_set_automation_track (Region * region, AutomationTrack * at);

/**
 * Gets the AutomationTrack using the saved index.
 *
 * @public @memberof Region
 */
AutomationTrack *
region_get_automation_track (const Region * const region);

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (Region * src, Region * dest);

/**
 * Returns if the position is inside the region
 * or not.
 *
 * FIXME move to arranger object
 *
 * @param gframes Global position in frames.
 * @param inclusive Whether the last frame should
 *   be counted as part of the region.
 *
 * @public @memberof Region
 */
NONNULL int
region_is_hit (
  const Region *       region,
  const signed_frame_t gframes,
  const bool           inclusive);

/**
 * Returns if any part of the Region is inside the
 * given range, inclusive.
 *
 * FIXME move to arranger object
 *
 * @public @memberof Region
 */
int
region_is_hit_by_range (
  const Region *       region,
  const signed_frame_t gframes_start,
  const signed_frame_t gframes_end,
  const bool           end_inclusive);

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param at The automation track to look in.
 * @param track The track to look in, if at is
 *   NULL.
 * @param pos The position.
 */
NONNULL_ARGS (3)
Region * region_at_position (
  const Track *           track,
  const AutomationTrack * at,
  const Position *        pos);

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 *
 * @public @memberof Region
 */
char *
region_generate_filename (Region * region);

void
region_get_type_as_string (RegionType type, char * buf);

/**
 * Returns if this region is currently being
 * recorded onto.
 *
 * @public @memberof Region
 */
bool
region_is_recording (Region * self);

/**
 * Returns whether the region is effectively in
 * musical mode.
 *
 * @note Only applicable to audio regions.
 *
 * @public @memberof Region
 */
bool
region_get_musical_mode (Region * self);

/**
 * Wrapper for adding an arranger object.
 *
 * @public @memberof Region
 */
void
region_add_arranger_object (Region * self, ArrangerObject * obj, bool fire_events);

void
region_create_link_group_if_none (Region * region);

/**
 * Removes the link group from the region, if any.
 *
 * @public @memberof Region
 */
void
region_unlink (Region * region);

/**
 * Removes all children objects from the region.
 *
 * @public @memberof Region
 */
void
region_remove_all_children (Region * region);

/**
 * Clones and copies all children from \ref src to
 * \ref dest.
 */
void
region_copy_children (Region * dest, Region * src);

NONNULL bool
region_is_looped (const Region * const self);

/**
 * Returns the ArrangerSelections based on the
 * given region type.
 *
 * @public @memberof Region
 */
ArrangerSelections *
region_get_arranger_selections (Region * self);

/**
 * Returns the arranger for editing the region's children.
 *
 * @public @memberof Region
 */
ArrangerWidget *
region_get_arranger_for_children (Region * self);

/**
 * Sanity checking.
 *
 * @param frames_per_tick Frames per tick used when validating audio regions.
 * Passing 0 will use the value from the current engine.
 *
 * @public @memberof Region
 */
bool
region_validate (Region * self, bool is_project, double frames_per_tick);

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the Region or its children's
 * resources.
 *
 * @public @memberof Region
 */
void
region_disconnect (Region * self);

/**
 * @}
 */

#endif // __AUDIO_REGION_H__
