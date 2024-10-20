// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_PORT_H__
#define __AUDIO_AUDIO_PORT_H__

#include "common/dsp/port.h"
#include "common/utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief Audio port specifics.
 */
class AudioPort final
    : public Port,
      public ICloneable<AudioPort>,
      public ISerializable<AudioPort>
{
public:
  AudioPort ();

  AudioPort (std::string label, PortFlow flow);

  bool has_sound () const override;

  static constexpr size_t AUDIO_RING_SIZE = 65536;

#if HAVE_RTAUDIO
  /**
   * Dequeue the audio data from the ring buffers into @ref RtAudioDevice.buf.
   */
  void prepare_rtaudio_data ();

  /**
   * Sums the inputs coming in from RtAudio before the port is processed.
   */
  void
  sum_data_from_rtaudio (const nframes_t start_frame, const nframes_t nframes);

  void expose_to_rtaudio (bool expose);
#endif

  /**
   * @brief Applies the fader to the audio buffer.
   *
   * @param amp The fader amplitude (0.0 to 1.0).
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_fader (float amp, nframes_t start_frame, const nframes_t nframes);

  /**
   * @brief Applies the pan to the audio buffer.
   *
   * @param pan The pan value (-1.0 to 1.0).
   * @param pan_law The pan law to use.
   * @param pan_algo The pan algorithm to use.
   * @param start_frame The start frame offset from 0 in this cycle.
   * @param nframes The number of frames to process.
   */
  void apply_pan (
    float           pan,
    PanLaw          pan_law,
    PanAlgorithm    pan_algo,
    nframes_t       start_frame,
    const nframes_t nframes);

  /**
   * @brief Returns the peak amplitude of the audio buffer.
   *
   * @return The peak amplitude (0.0 to 1.0).
   */
  float get_peak () const { return peak_; }

  /**
   * @brief Resets the peak amplitude to 0.
   */
  void reset_peak () { peak_ = 0.f; }

  void
  process (const EngineProcessTimeInfo time_nfo, const bool noroll) override;

  void allocate_bufs () override;

  void clear_buffer (AudioEngine &engine) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  bool is_stereo_port () const;

#if HAVE_JACK

  /**
   * Receives audio data from the port's exposed JACK port (if any) into the
   * port.
   */
  void receive_audio_data_from_jack (
    const nframes_t start_frames,
    const nframes_t nframes);

  /**
   * Pastes the audio data in the port starting at @p start_frames to the JACK
   * port starting at
   * @p start_frames.
   */
  void
  send_audio_data_to_jack (const nframes_t start_frames, const nframes_t nframes);
#endif

  void init_after_cloning (const AudioPort &other) override;

private:
  /**
   * Sums the inputs coming in from the dummy engine StereoPorts, before the
   * port is processed.
   */
  void
  sum_data_from_dummy (const nframes_t start_frame, const nframes_t nframes);

public:
#if HAVE_RTAUDIO
  /**
   * RtAudio pointers for input ports.
   *
   * Each port can have multiple RtAudio devices.
   */
  std::vector<std::shared_ptr<RtAudioDevice>> rtaudio_ins_;
#else
  std::vector<std::shared_ptr<int>> rtaudio_ins_;
#endif

private:
  /** Max amplitude during processing (fabsf). */
  float peak_ = 0.f;

  /** Last time @ref peak_ was set. */
  qint64 peak_timestamp_ = 0;
};

/**
 * L & R port, for convenience.
 */
class StereoPorts final
    : public ICloneable<StereoPorts>,
      public ISerializable<StereoPorts>
{
public:
  StereoPorts () = default;
  StereoPorts (const AudioPort &l, const AudioPort &r);

  StereoPorts (std::unique_ptr<AudioPort> &&l, std::unique_ptr<AudioPort> &&r)
      : l_ (std::move (l)), r_ (std::move (r))
  {
  }

  /**
   * Creates stereo ports for generic use.
   *
   * @param input Whether input ports.
   * @param owner Pointer to the owner. The type is determined by owner_type.
   */
  StereoPorts (bool input, std::string name, std::string symbol);

  ~StereoPorts () override;

  template <typename T> void init_loaded (T * owner)
  {
    l_->init_loaded (owner);
    r_->init_loaded (owner);
  }

  template <typename T> void set_owner (T * owner)
  {
    l_->set_owner (owner);
    r_->set_owner (owner);
  }

  void set_expose_to_backend (bool expose)
  {
    l_->set_expose_to_backend (expose);
    r_->set_expose_to_backend (expose);
  }

  void disconnect_hw_inputs ()
  {
    l_->disconnect_hw_inputs ();
    r_->disconnect_hw_inputs ();
  }

  void clear_buffer (AudioEngine &engine)
  {
    l_->clear_buffer (engine);
    r_->clear_buffer (engine);
  }

  void allocate_bufs ()
  {
    l_->allocate_bufs ();
    r_->allocate_bufs ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

  /**
   * Connects to the given port using Port::connect().
   *
   * @param dest Destination port.
   * @param locked Lock the connection.
   */
  ATTR_NONNULL void connect_to (StereoPorts &dest, bool locked);

  void disconnect ();

  void set_write_ring_buffers (bool on)
  {
    l_->write_ring_buffers_ = on;
    r_->write_ring_buffers_ = on;
  }

  AudioPort       &get_l () { return *l_; }
  AudioPort       &get_r () { return *r_; }
  const AudioPort &get_l () const { return *l_; }
  const AudioPort &get_r () const { return *r_; }

  void init_after_cloning (const StereoPorts &other) override
  {
    l_ = other.l_->clone_unique ();
    r_ = other.r_->clone_unique ();
  }

private:
  std::unique_ptr<AudioPort> l_; ///< Left port
  std::unique_ptr<AudioPort> r_; ///< Right port
};

/**
 * @}
 */

#endif