// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/fadeable_object.h"

namespace zrythm::structure::arrangement
{
ArrangerObjectFadeRange::ArrangerObjectFadeRange (
  const dsp::AtomicPosition::TimeConversionFunctions &time_conversion_funcs,
  QObject *                                           parent)
    : QObject (parent), start_offset_ (time_conversion_funcs),
      start_offset_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          start_offset_,
          // Fade offsets must be non-negative
          [] (units::precise_tick_t ticks) {
            return std::max (ticks, units::ticks (0.0));
          },
          this)),
      end_offset_ (time_conversion_funcs),
      end_offset_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (
          end_offset_,
          // Fade offsets must be non-negative
          [] (units::precise_tick_t ticks) {
            return std::max (ticks, units::ticks (0.0));
          },
          this)),
      fade_in_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_in_opts_, this)),
      fade_out_opts_adapter_ (
        utils::make_qobject_unique<
          dsp::CurveOptionsQmlAdapter> (fade_out_opts_, this))
{
  QObject::connect (
    startOffset (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    endOffset (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeInCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::algorithmChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
  QObject::connect (
    fadeOutCurveOpts (), &dsp::CurveOptionsQmlAdapter::curvinessChanged, this,
    &ArrangerObjectFadeRange::fadePropertiesChanged);
}
}
