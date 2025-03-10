// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/region_identifier.h"

bool
RegionIdentifier::validate () const
{
  return !track_uuid_.is_null ();
}
