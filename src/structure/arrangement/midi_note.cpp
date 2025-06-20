// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/velocity.h"
#include "structure/tracks/piano_roll_track.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{
MidiNote::MidiNote (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (Type::MidiNote, track_resolver), QObject (parent),
      RegionOwnedObject (obj_registry), vel_ (new Velocity (this))
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
}

ArrangerObjectPtrVariant
MidiNote::add_clone_to_project (bool fire_events) const
{
  return {};
#if 0
  auto ret = clone_raw_ptr ();
  get_region ()->append_object (ret, true);
  return ret;
#endif
}

ArrangerObjectPtrVariant
MidiNote::insert_clone_to_project () const
{
  return {};
// TODO
#if 0
  auto ret = clone_raw_ptr ();
  get_region ()->insert_object (ret, index_, true);
  return ret;
#endif
}

utils::Utf8String
MidiNote::gen_human_friendly_name () const
{
  return get_val_as_string (Notation::Musical, false);
}

void
MidiNote::listen (bool listen)
{
  /*z_info (*/
  /*"%s: %" PRIu8 " listen %d", __func__,*/
  /*mn->val, listen);*/

  auto track_var = get_track ();
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
        {
          auto &events =
            track->processor_->get_midi_in_port ().midi_events_.queued_events_;

          if (listen)
            {
              /* if note is on but pitch changed */
              if (currently_listened_ && pitch_ != last_listened_pitch_)
                {
                  /* create midi note off */
                  events.add_note_off (1, last_listened_pitch_, 0);

                  /* create note on at the new value */
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
                  last_listened_pitch_ = pitch_;
                }
              /* if note is on and pitch is the same */
              else if (currently_listened_ && pitch_ == last_listened_pitch_)
                {
                  /* do nothing */
                }
              /* if note is not on */
              else if (!currently_listened_)
                {
                  /* turn it on */
                  events.add_note_on (1, pitch_, vel_->vel_, 0);
                  last_listened_pitch_ = pitch_;
                  currently_listened_ = 1;
                }
            }
          /* if turning listening off */
          else if (currently_listened_)
            {
              /* create midi note off */
              events.add_note_off (1, last_listened_pitch_, 0);
              currently_listened_ = false;
              last_listened_pitch_ = 255;
            }
        }
    },
    track_var);
}

utils::Utf8String
MidiNote::get_val_as_string (Notation notation, bool use_markup) const
{
  const auto note_str_musical = dsp::ChordDescriptor::note_to_string (
    ENUM_INT_TO_VALUE (dsp::MusicalNote, pitch_ % 12));
  utils::Utf8String note_str;
  if (notation == Notation::Musical)
    {
      note_str = note_str_musical;
    }
  else
    {
      note_str = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("{}", pitch_));
    }

  const int note_val = pitch_ / 12 - 1;
  if (use_markup)
    {
      auto buf = fmt::format ("{}<sup>{}</sup>", note_str, note_val);
      if (DEBUGGING)
        {
          buf += fmt::format (" ({})", get_uuid ());
        }
      return utils::Utf8String::from_utf8_encoded_string (buf);
    }

  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}{}", note_str, note_val));
}

void
MidiNote::set_pitch (const uint8_t val)
{
  z_return_if_fail (val < 128);

  /* if currently playing set a note off event. */
  if (
    is_hit (TRANSPORT->get_playhead_position_in_gui_thread ().frames_)
    && TRANSPORT->isRolling ())
    {
      auto region = dynamic_cast<MidiRegion *> (get_region ());
      z_return_if_fail (region);
      auto track_var = region->get_track ();
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, tracks::PianoRollTrack>)
            {
              auto &midi_events =
                track->processor_->get_piano_roll_port ()
                  .midi_events_.queued_events_;

              uint8_t midi_ch = region->get_midi_ch ();
              midi_events.add_note_off (midi_ch, pitch_, 0);
            }
        },
        track_var);
    }

  pitch_ = val;
}

void
MidiNote::shift_pitch (const int delta)
{
  pitch_ = (uint8_t) ((int) pitch_ + delta);
  set_pitch (pitch_);
}

void
init_from (MidiNote &obj, const MidiNote &other, utils::ObjectCloneType clone_type)
{
  obj.pitch_ = other.pitch_;
  obj.vel_->setValue (other.vel_->getValue ());
  obj.currently_listened_ = other.currently_listened_;
  obj.last_listened_pitch_ = other.last_listened_pitch_;
  obj.vel_->vel_at_start_ = other.vel_->vel_at_start_;
  init_from (
    static_cast<BoundedObject &> (obj),
    static_cast<const BoundedObject &> (other), clone_type);
  init_from (
    static_cast<MuteableObject &> (obj),
    static_cast<const MuteableObject &> (other), clone_type);
  init_from (
    static_cast<RegionOwnedObject &> (obj),
    static_cast<const RegionOwnedObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

bool
MidiNote::validate (bool is_project, dsp::FramesPerTick frames_per_tick) const
{
  // TODO
  return true;
}
}
