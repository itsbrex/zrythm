// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/region.h"
#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

ChordSelections::ChordSelections () : ArrangerSelections (Type::Chord) { }

bool
ChordSelections::can_be_pasted_at_impl (const Position pos, const int idx) const
{
  Region * r = CLIP_EDITOR->get_region ();
  if (!r || !r->is_chord ())
    return false;

  if (r->pos_.frames_ + pos.frames_ < 0)
    return false;

  return true;
}

void
ChordSelections::sort_by_indices (bool desc)
{
  std::sort (
    objects_.begin (), objects_.end (), [desc] (const auto &a, const auto &b) {
      bool ret = false;
      ret =
        dynamic_cast<ChordObject &> (*a).index_
        < dynamic_cast<ChordObject &> (*b).index_;
      return desc ? !ret : ret;
    });
}