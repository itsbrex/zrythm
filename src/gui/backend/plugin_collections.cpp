// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/plugin_collections.h"
#include "gui/backend/zrythm_application.h"
#include "utils/directory_manager.h"
#include "utils/io.h"
#include "utils/utf8_string.h"

constexpr const char * PLUGIN_COLLECTIONS_JSON_FILENAME =
  "plugin-collections.json";

namespace zrythm::gui::old_dsp::plugins
{

bool
PluginCollection::contains_descriptor (
  const zrythm::plugins::PluginDescriptor &descr) const
{
  return std::ranges::any_of (descriptors_, [&descr] (const auto &cur_descr) {
    return descr.is_same_plugin (*cur_descr);
  });
}

void
PluginCollection::add_descriptor (const zrythm::plugins::PluginDescriptor &descr)
{
  if (contains_descriptor (descr))
    {
      return;
    }

  auto new_descr = utils::clone_unique (descr);
  descriptors_.emplace_back (std::move (new_descr));
}

void
PluginCollection::remove_descriptor (
  const zrythm::plugins::PluginDescriptor &descr)
{
  std::erase_if (descriptors_, [&descr] (const auto &cur_descr) {
    return cur_descr->is_same_plugin (descr);
  });
}

#if 0
Glib::RefPtr<Gio::MenuModel>
PluginCollection::generate_context_menu () const
{
  auto menu = Gio::Menu::create ();

  // TODO icon name edit-rename
  menu->append (QObject::tr ("Rename"), "app.plugin-collection-rename");

  // TODO icon name edit-delete
  menu->append (QObject::tr ("Delete"), "app.plugin-collection-remove");

  return menu;
}
#endif

void
init_from (
  PluginCollection       &obj,
  const PluginCollection &other,
  utils::ObjectCloneType  clone_type)
{
  obj.name_ = other.name_;
  obj.description_ = other.description_;
  utils::clone_unique_ptr_container (obj.descriptors_, other.descriptors_);
}

/* ============================================================================ */

fs::path
PluginCollections::get_file_path ()
{
  auto zrythm_dir =
    dynamic_cast<ZrythmApplication *> (qApp)->get_directory_manager ().get_dir (
      IDirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / PLUGIN_COLLECTIONS_JSON_FILENAME;
}

void
PluginCollections::serialize_to_file () const
{
  z_info ("Serializing plugin collections...");
  nlohmann::json json = *this;
  auto           path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  z_debug ("Writing plugin collections to {}...", path);
  try
    {
      utils::io::set_file_contents (
        path, utils::Utf8String::from_utf8_encoded_string (json.dump ()));
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (
        format_str ("Unable to write plugin collections: {}", e.what ()));
    }
}

void
PluginCollections::delete_file ()
{
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Plugin collections file at {} does not exist", path);
      return;
    }
  z_debug ("Deleting plugin collections file at {}", path);
  try
    {
      fs::remove (path);
    }
  catch (const fs::filesystem_error &e)
    {
      z_warning ("Failed to delete plugin collections file: {}", e.what ());
    }
}

std::unique_ptr<PluginCollections>
PluginCollections::read_or_new ()
{
  auto ret = std::make_unique<PluginCollections> ();
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Plugin collections file at {} does not exist", path);
      return ret;
    }

  std::string json;
  try
    {
      json =
        utils::Utf8String::from_qstring (utils::io::read_file_contents (path))
          .str ();
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create PluginCollections from {}", path);
      return ret;
    }

  try
    {
      auto j = nlohmann::json::parse (json);
      from_json (j, *ret);
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "Found invalid cached plugin descriptor file (error: {}). "
        "Purging file and creating a new one.",
        e.what ());

      try
        {
          delete_file ();
        }
      catch (const ZrythmException &e2)
        {
          z_warning (e2.what ());
        }

      return {};
    }

  return ret;
}

void
PluginCollections::add (
  const zrythm::gui::old_dsp::plugins::PluginCollection &collection,
  bool                                                   serialize)
{
  collections_.emplace_back (utils::clone_unique (collection));

  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}

const zrythm::gui::old_dsp::plugins::PluginCollection *
PluginCollections::find_from_name (std::string_view name) const
{
  auto it = std::ranges::find_if (collections_, [&name] (const auto &collection) {
    return collection->name_ == name;
  });

  return it == collections_.end () ? nullptr : (*it).get ();
}

void
PluginCollections::remove (PluginCollection &collection, bool serialize)
{
  std::erase_if (collections_, [&collection] (const auto &cur_collection) {
    return cur_collection->name_ == collection.name_;
  });

  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}
}
