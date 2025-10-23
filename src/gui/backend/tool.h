// SPDX-FileCopyrightText: © 2018-2019, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"
#include "utils/serialization.h"

#include <QObject>
#include <QtQmlIntegration>

namespace zrythm::gui::backend
{

class Tool : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    int toolValue READ getToolValue WRITE setToolValue NOTIFY toolValueChanged)

public:
  enum ToolType
  {
    Select,
    Edit,
    Cut,
    Eraser,
    Ramp,
    Audition,
  };
  Q_ENUM (ToolType)

  Tool (QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (Tool)
  ~Tool () override;

  [[nodiscard]] int getToolValue () const;
  void              setToolValue (int tool);
  Q_SIGNAL void     toolValueChanged (int tool);

  friend void
  init_from (Tool &obj, const Tool &other, utils::ObjectCloneType clone_type);

private:
  static constexpr auto kToolValueKey = "toolValue"sv;
  friend void           to_json (nlohmann::json &j, const Tool &tool)
  {
    j[kToolValueKey] = tool.tool_;
  }
  friend void from_json (const nlohmann::json &j, Tool &tool)
  {
    j.at (kToolValueKey).get_to (tool.tool_);
  }

private:
  ToolType tool_{ ToolType::Select };
};

}; // namespace zrythm::gui::backend
