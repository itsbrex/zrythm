// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/utf8_string.h"

namespace zrythm::dsp
{
Q_NAMESPACE

enum class MusicalNote : std::uint8_t
{
  C = 0,
  CSharp,
  D,
  DSharp,
  E,
  F,
  FSharp,
  G,
  GSharp,
  A,
  ASharp,
  B
};
Q_ENUM_NS (MusicalNote)

/**
 * Chord type.
 */
enum class ChordType : std::uint8_t
{
  None,
  Major,
  Minor,
  Diminished,
  SuspendedFourth,
  SuspendedSecond,
  Augmented,
  Custom,
};
Q_ENUM_NS (ChordType)

/**
 * Chord accents.
 */
enum class ChordAccent : std::uint8_t
{
  None,
  /** b7 is 10 semitones from chord root, or 9
   * if the chord is diminished. */
  Seventh,
  /** Maj7 is 11 semitones from the root. */
  MajorSeventh,
  /* NOTE: all accents below assume Seventh */
  /** 13 semitones. */
  FlatNinth,
  /** 14 semitones. */
  Ninth,
  /** 15 semitones. */
  SharpNinth,
  /** 17 semitones. */
  Eleventh,
  /** 6 and 18 semitones. */
  FlatFifthSharpEleventh,
  /** 8 and 16 semitones. */
  SharpFifthFlatThirteenth,
  /** 9 and 21 semitones. */
  SixthThirteenth,
};
Q_ENUM_NS (ChordAccent)

/**
 * A ChordDescriptor describes a chord and is not linked to any specific object
 * by itself.
 *
 * Chord objects should include a ChordDescriptor.
 */
class ChordDescriptor
{
public:
  static constexpr size_t MAX_NOTES = 48;

public:
  ChordDescriptor () = default;

  /**
   * Creates a ChordDescriptor.
   */
  ChordDescriptor (
    MusicalNote root,
    bool        has_bass,
    MusicalNote bass,
    ChordType   type,
    ChordAccent accent,
    int         inversion)
      : has_bass_ (has_bass), root_note_ (root), bass_note_ (bass),
        type_ (type), accent_ (accent), inversion_ (inversion)
  {
    update_notes ();
  }

  int get_max_inversion () const
  {
    int max_inv = 2;
    switch (accent_)
      {
      case ChordAccent::None:
        break;
      case ChordAccent::Seventh:
      case ChordAccent::MajorSeventh:
      case ChordAccent::FlatNinth:
      case ChordAccent::Ninth:
      case ChordAccent::SharpNinth:
      case ChordAccent::Eleventh:
        max_inv = 3;
        break;
      case ChordAccent::FlatFifthSharpEleventh:
      case ChordAccent::SharpFifthFlatThirteenth:
      case ChordAccent::SixthThirteenth:
        max_inv = 4;
        break;
      default:
        break;
      }

    return max_inv;
  }

  int get_min_inversion () const { return -get_max_inversion (); }

  /**
   * Returns if the given key is in the chord represented by the given
   * ChordDescriptor.
   *
   * @param key A note inside a single octave (0-11).
   */
  bool is_key_in_chord (MusicalNote key) const;

  /**
   * Returns if @ref key is the bass or root note of @ref chord.
   *
   * @param key A note inside a single octave (0-11).
   */
  bool is_key_bass (MusicalNote key) const;

  /**
   * Returns the chord type as a string (eg. "aug").
   */
  static utils::Utf8String chord_type_to_string (ChordType type);

  /**
   * Returns the chord accent as a string (eg. "j7").
   */
  static utils::Utf8String chord_accent_to_string (ChordAccent accent);

  /**
   * Returns the musical note as a string (eg. "C3").
   */
  static utils::Utf8String note_to_string (MusicalNote note);

  /**
   * Returns the chord in human readable string.
   */
  utils::Utf8String to_string () const;

  /**
   * Updates the notes array based on the current
   * settings.
   */
  void update_notes ();

  friend bool
  operator== (const ChordDescriptor &lhs, const ChordDescriptor &rhs)
  {
    return lhs.has_bass_ == rhs.has_bass_ && lhs.root_note_ == rhs.root_note_
           && lhs.bass_note_ == rhs.bass_note_ && lhs.type_ == rhs.type_
           && lhs.notes_ == rhs.notes_ && lhs.inversion_ == rhs.inversion_;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (
    ChordDescriptor,
    has_bass_,
    root_note_,
    bass_note_,
    type_,
    accent_,
    notes_,
    inversion_)

public:
  /** Has bass note or not. */
  bool has_bass_ = false;

  /** Root note. */
  MusicalNote root_note_ = MusicalNote::C;

  /** Bass note 1 octave below. */
  MusicalNote bass_note_ = MusicalNote::C;

  /** Chord type. */
  ChordType type_ = ChordType::None;

  /**
   * Chord accent.
   *
   * Does not apply to custom chords.
   */
  ChordAccent accent_ = ChordAccent::None;

  /**
   * Only used if custom chord.
   *
   * 4 octaves, 1st octave is where bass note is, but bass note should not be
   * part of this.
   *
   * Starts at C always, from MIDI pitch 36.
   */
  std::array<bool, MAX_NOTES> notes_{};

  /**
   * 0 no inversion,
   * less than 0 highest note(s) drop an octave,
   * greater than 0 lowest note(s) receive an octave.
   */
  int inversion_ = 0;
};

} // namespace zrythm::dsp
