// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_BUS_TRACK_H__
#define __AUDIO_MIDI_BUS_TRACK_H__

#include "gui/dsp/channel_track.h"

/**
 * @class MidiBusTrack
 * @brief A track that processes MIDI data.
 *
 * The MidiBusTrack class is a concrete implementation of the ProcessableTrack,
 * ChannelTrack, and AutomatableTrack interfaces. It represents a track that
 * processes MIDI data.
 */
class MidiBusTrack final
    : public QObject,
      public ChannelTrack,
      public ICloneable<MidiBusTrack>,
      public zrythm::utils::serialization::ISerializable<MidiBusTrack>,
      public InitializableObjectFactory<MidiBusTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiBusTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MidiBusTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiBusTrack)

  friend class InitializableObjectFactory<MidiBusTrack>;

public:
  void init_after_cloning (const MidiBusTrack &other) override;

  void init_loaded () override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  MidiBusTrack (const std::string &name = "", int pos = 0);

  bool initialize () override;
};

#endif // __AUDIO_MIDI_BUS_TRACK_H__
