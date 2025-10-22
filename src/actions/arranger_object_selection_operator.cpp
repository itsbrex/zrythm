// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_selection_operator.h"
#include "commands/move_arranger_objects_command.h"
#include "utils/math.h"

namespace zrythm::actions
{
ArrangerObjectSelectionOperator::ArrangerObjectSelectionOperator (
  QObject * parent)
    : QObject (parent)
{
}

bool
ArrangerObjectSelectionOperator::moveByTicks (double tick_delta)
{
  // Validation checks
  if (undo_stack_ == nullptr)
    {
      qWarning () << "UndoStack not set for ArrangerObjectSelectionOperator";
      return false;
    }

  if (selection_model_ == nullptr)
    {
      qWarning ()
        << "SelectionModel not set for ArrangerObjectSelectionOperator";
      return false;
    }

  if (tick_delta == 0.0)
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      qWarning () << "No objects selected for movement";
      return false;
    }

  // Validate object bounds (don't move objects before timeline start)
  if (!validateHorizontalMovement (selected_objects, tick_delta))
    {
      qWarning ()
        << "Movement validation failed - objects would be moved before timeline start";
      return false;
    }

  // Create and push command
  auto * command = new commands::MoveArrangerObjectsCommand (
    std::move (selected_objects), units::ticks (tick_delta));
  undo_stack_->push (command);

  return true;
}

bool
ArrangerObjectSelectionOperator::moveNotesByPitch (int pitch_delta)
{
  return process_vertical_move (static_cast<double> (pitch_delta));
}

bool
ArrangerObjectSelectionOperator::moveAutomationPointsByDelta (double delta)
{
  return process_vertical_move (delta);
}

bool
ArrangerObjectSelectionOperator::process_vertical_move (double delta)
{
  // Validation checks
  if (undo_stack_ == nullptr)
    {
      qWarning () << "UndoStack not set for ArrangerObjectSelectionOperator";
      return false;
    }

  if (selection_model_ == nullptr)
    {
      qWarning ()
        << "SelectionModel not set for ArrangerObjectSelectionOperator";
      return false;
    }

  if (utils::math::floats_equal (delta, 0.0))
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      qWarning () << "No objects selected for movement";
      return false;
    }

  // Validate object vertical bounds
  if (!validateVerticalMovement (selected_objects, delta))
    {
      qWarning ()
        << "Movement validation failed - objects would be moved outside bounds";
      return false;
    }

  // Create and push command
  auto * command = new commands::MoveArrangerObjectsCommand (
    std::move (selected_objects), units::ticks (0), delta);
  undo_stack_->push (command);

  return true;
}

auto
ArrangerObjectSelectionOperator::extractSelectedObjects () const
  -> SelectedObjectsVector
{
  SelectedObjectsVector objects;

  if (selection_model_ == nullptr)
    {
      return objects;
    }

  const auto selected_indexes = selection_model_->selectedIndexes ();
  for (const auto &index : selected_indexes)
    {
      // Get the object from the model index
      auto variant = index.data (
        structure::arrangement::ArrangerObjectListModel::
          ArrangerObjectUuidReferenceRole);
      if (
        variant
          .canConvert<structure::arrangement::ArrangerObjectUuidReference *> ())
        {
          auto * obj_ref = variant.value<
            structure::arrangement::ArrangerObjectUuidReference *> ();
          objects.push_back (*obj_ref);
        }
    }

  return objects;
}

bool
ArrangerObjectSelectionOperator::validateHorizontalMovement (
  const SelectedObjectsVector &objects,
  double                       tick_delta)
{
  return std::ranges::all_of (objects, [tick_delta] (const auto &obj_ref) {
    if (auto * obj = obj_ref.get_object_base ())
      {
        const double new_position = obj->position ()->ticks () + tick_delta;
        const auto   timeline_position = [&] () {
          const auto * parent_obj = obj->parentObject ();
          if (parent_obj != nullptr)
            {
              return new_position + parent_obj->position ()->ticks ();
            }
          return new_position;
        }();
        return timeline_position >= 0.0;
      }
    return true; // Skip objects that can't be accessed
  });
}

bool
ArrangerObjectSelectionOperator::validateVerticalMovement (
  const SelectedObjectsVector &objects,
  double                       delta)
{
  return std::ranges::all_of (objects, [delta] (const auto &obj_ref) {
    return std::visit (
      [&] (auto &&obj) {
        using ObjectT = base_type<decltype (obj)>;
        if constexpr (std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
          {
            const auto new_pitch = obj->pitch () + static_cast<int> (delta);
            return new_pitch >= 0 && new_pitch < 128;
          }
        if constexpr (
          std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
          {
            const auto new_value = obj->value () + static_cast<float> (delta);
            return new_value >= 0.0 && new_value <= 1.0;
          }

        return true; // Skip objects that can't be accessed
      },
      obj_ref.get_object ());
  });
}

} // namespace zrythm::actions
