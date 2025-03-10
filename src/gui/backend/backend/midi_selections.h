// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * API for selections in the piano roll.
 */

#ifndef __GUI_BACKEND_MIDI_SELECTIONS_H__
#define __GUI_BACKEND_MIDI_SELECTIONS_H__

#include "gui/backend/backend/arranger_selections.h"
#include "gui/dsp/midi_note.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define MIDI_SELECTIONS (PROJECT->midi_selections_)

/**
 * A collection of selected MidiNote's.
 *
 * Used for copying, undoing, etc.
 */
class MidiSelections final
    : public QObject,
      public ArrangerSelections,
      public ICloneable<MidiSelections>,
      public zrythm::utils::serialization::ISerializable<MidiSelections>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_SELECTIONS_QML_PROPERTIES (MidiSelections)

public:
  MidiSelections (QObject * parent = nullptr);

  MidiNote * get_highest_note ();

  MidiNote * get_lowest_note ();

  /**
   * Sets the listen status of notes on and off based on changes in the previous
   * selections and the current selections.
   */
  void unlisten_note_diff (const MidiSelections &prev);

  void sort_by_pitch (bool desc);

  void sort_by_indices (bool desc) override;

  void
  init_after_cloning (const MidiSelections &other, ObjectCloneType clone_type)
    override
  {
    ArrangerSelections::copy_members_from (other, clone_type);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool can_be_pasted_at_impl (Position pos, int idx) const override;
};

/**
 * @}
 */

#endif
