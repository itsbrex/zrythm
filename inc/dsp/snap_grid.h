// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Snap/grid information.
 */

#ifndef __AUDIO_SNAP_GRID_H__
#define __AUDIO_SNAP_GRID_H__

#include "dsp/position.h"

#include <glib/gi18n.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

#define SNAP_GRID_TIMELINE (PROJECT->snap_grid_timeline)
#define SNAP_GRID_EDITOR (PROJECT->snap_grid_editor)

#define SNAP_GRID_IS_EDITOR(sg) (SNAP_GRID_EDITOR == sg)
#define SNAP_GRID_IS_TIMELINE(sg) (SNAP_GRID_TIMELINE == sg)
/* if any snapping is enabled */
#define SNAP_GRID_ANY_SNAP(sg) (sg->snap_to_grid || sg->snap_to_events)
#define SNAP_GRID_DEFAULT_MAX_BAR 10000

enum class NoteLength
{
  NOTE_LENGTH_BAR,
  NOTE_LENGTH_BEAT,
  NOTE_LENGTH_2_1,
  NOTE_LENGTH_1_1,
  NOTE_LENGTH_1_2,
  NOTE_LENGTH_1_4,
  NOTE_LENGTH_1_8,
  NOTE_LENGTH_1_16,
  NOTE_LENGTH_1_32,
  NOTE_LENGTH_1_64,
  NOTE_LENGTH_1_128
};

const char **
note_length_get_strings (void);
const char *
note_length_to_str (NoteLength len);

enum class NoteType
{
  NOTE_TYPE_NORMAL,
  NOTE_TYPE_DOTTED, ///< 2/3 of its original size
  NOTE_TYPE_TRIPLET ///< 3/2 of its original size
};

const char **
note_type_get_strings (void);
const char *
note_type_to_str (NoteType type);

enum class NoteLengthType
{
  /** Link length with snap setting. */
  NOTE_LENGTH_LINK,

  /** Use last created object's length. */
  NOTE_LENGTH_LAST_OBJECT,

  /** Custom length. */
  NOTE_LENGTH_CUSTOM,
};

/**
 * Snap grid type.
 */
enum class SnapGridType
{
  SNAP_GRID_TYPE_TIMELINE,
  SNAP_GRID_TYPE_EDITOR,
};

typedef struct SnapGrid
{
  SnapGridType type;

  /**
   * If this is on, the snap note length will be determined
   * automatically based on the current zoom level.
   *
   * The snap note type still applies.
   */
  bool snap_adaptive;

  /** Snap note length. */
  NoteLength snap_note_length;

  /** Snap note type. */
  NoteType snap_note_type;

  /** Whether to snap to the grid. */
  bool snap_to_grid;

  /**
   * Whether to keep the offset when moving items.
   *
   * This requires @ref SnapGrid.snap_to_grid to be
   * enabled.
   */
  bool snap_to_grid_keep_offset;

  /** Whether to snap to events. */
  bool snap_to_events;

  /** Default note length. */
  NoteLength default_note_length;
  /** Default note type. */
  NoteType default_note_type;

  /**
   * If this is on, the default note length will be
   * determined automatically based on the current
   * zoom level.
   *
   * The default note type still applies.
   *
   * TODO this will be done after v1.
   */
  bool default_adaptive;

  /**
   * See NoteLengthType.
   */
  NoteLengthType length_type;
} SnapGrid;

void
snap_grid_init (
  SnapGrid *   self,
  SnapGridType type,
  NoteLength   note_length,
  bool         adaptive);

int
snap_grid_get_ticks_from_length_and_type (NoteLength length, NoteType type);

/**
 * Gets a snap point's length in ticks.
 */
NONNULL int
snap_grid_get_snap_ticks (const SnapGrid * self);

NONNULL double
snap_grid_get_snap_frames (const SnapGrid * self);

/**
 * Gets a the default length in ticks.
 */
int
snap_grid_get_default_ticks (SnapGrid * self);

/**
 * Returns the grid intensity as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize_length_and_type (NoteLength note_length, NoteType note_type);

/**
 * Returns the grid intensity as a human-readable
 * string.
 *
 * Must be free'd.
 */
char *
snap_grid_stringize (SnapGrid * self);

/**
 * Returns the next or previous SnapGrid point.
 *
 * Must not be free'd.
 *
 * @param self Snap grid to search in.
 * @param pos Position to search for.
 * @param return_prev 1 to return the previous
 * element or 0 to return the next.
 *
 * @return Whether successful.
 */
NONNULL bool
snap_grid_get_nearby_snap_point (
  Position *             ret_pos,
  const SnapGrid * const self,
  const Position *       pos,
  const bool             return_prev);

SnapGrid *
snap_grid_clone (SnapGrid * src);

SnapGrid *
snap_grid_new (void);

void
snap_grid_free (SnapGrid * self);

/**
 * @}
 */

#endif
