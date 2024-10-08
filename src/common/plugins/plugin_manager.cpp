// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2008-2012 Paul Davis
 * Copyright (C) David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include "common/plugins/cached_plugin_descriptors.h"
#include "common/plugins/carla_discovery.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/directory_manager.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/windows.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

#include <QtConcurrent>

#include <glib/gi18n.h>

using namespace zrythm::plugins;

PluginManager::PluginManager (QObject * parent)
    : QObject (parent),
      plugin_descriptors_ (std::make_unique<gui::PluginDescriptorList> ()),
      cached_plugin_descriptors_ (CachedPluginDescriptors::read_or_new ()),
      collections_ (PluginCollections::read_or_new ()),
      scanner_ (std::make_unique<PluginScanner> ())
#if HAVE_CARLA
      ,
      carla_discovery_ (std::make_unique<ZCarlaDiscovery> (*this))
#endif
{
}

void
PluginManager::add_category_and_author (
  std::string_view category,
  std::string_view author)
{
  if (!string_is_ascii (category))
    {
      z_warning ("Ignoring non-ASCII plugin category name...");
    }
  if (
    !std::any_of (
      plugin_categories_.begin (), plugin_categories_.end (),
      [&] (const auto &cat) { return cat == category; }))
    {
      z_debug ("New category: {}", category);
      plugin_categories_.push_back (std::string (category));
    }

  if (!author.empty ())
    {
      if (
        !std::any_of (
          plugin_authors_.begin (), plugin_authors_.end (),
          [&] (const auto &cur_author) { return cur_author == author; }))
        {
          z_debug ("New author: {}", author);
          plugin_authors_.emplace_back (author);
        }
    }
}

static void
add_expanded_paths (StringArray &arr, const QStringList &paths_from_settings)
{
  for (const auto &path : paths_from_settings)
    {
      auto expanded_cur_path = string_expand_env_vars (path.toStdString ());
      /* split because the env might contain multiple paths */
      auto expanded_paths = Glib::Regex::split_simple (
        G_SEARCHPATH_SEPARATOR_S, expanded_cur_path.c_str ());
      for (auto &expanded_path : expanded_paths)
        {
          arr.add (expanded_path);
        }
    }
}

StringArray
PluginManager::get_lv2_paths ()
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      /* add test plugins if testing */
      auto tests_builddir = Glib::getenv ("G_TEST_BUILDDIR");
      auto root_builddir = Glib::getenv ("G_TEST_BUILD_ROOT_DIR");
      z_return_val_if_fail (!tests_builddir.empty (), nullptr);
      z_return_val_if_fail (!root_builddir.empty (), nullptr);

      auto test_lv2_plugins = fs::path (tests_builddir) / "lv2plugins";
      auto test_root_plugins = fs::path (root_builddir) / "data" / "plugins";
      ret.add (test_lv2_plugins.string ());
      ret.add (test_root_plugins.string ());

      QStringList paths_from_settings = { "${LV2_PATH}", "/usr/lib/lv2" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("LV2 paths");

      return ret;
    }

  auto paths_from_settings =
    zrythm::gui::SettingsManager::get_instance ()->get_lv2_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret.add ("C:\\Program Files\\Common Files\\LV2");
#elif defined(__APPLE__)
      ret.add ("/Library/Audio/Plug-ins/LV2");
#elif defined(FLATPAK_BUILD)
      ret.add ("/app/lib/lv2");
      ret.add ("/app/extensions/Plugins/lv2");
#else /* non-flatpak UNIX */
      {
        auto home_lv2 = fs::path (Glib::get_home_dir ()) / ".lv2";
        ret.add (home_lv2);
      }
      ret.add ("/usr/lib/lv2");
      ret.add ("/usr/local/lib/lv2");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/lv2");
      ret.add ("/usr/local/lib64/lv2");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/lv2");
          ret.add ("/usr/local/" LIBDIR_NAME "/lv2");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  /* add special paths */
  auto * dir_mgr = DirectoryManager::getInstance ();
  auto   builtin_plugins_path = dir_mgr->get_dir (
    DirectoryManager::DirectoryType::SYSTEM_BUNDLED_PLUGINSDIR);
  auto special_plugins_path = dir_mgr->get_dir (
    DirectoryManager::DirectoryType::SYSTEM_SPECIAL_LV2_PLUGINS_DIR);
  ret.add (builtin_plugins_path);
  ret.add (special_plugins_path);

  ret.print ("LV2 paths");

  return ret;
}

StringArray
PluginManager::get_vst2_paths ()
{
  StringArray ret;

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${VST_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("VST2 paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_vst2_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret.add ("C:\\Program Files\\Common Files\\VST2");
      ret.add ("C:\\Program Files\\VSTPlugins");
      ret.add ("C:\\Program Files\\Steinberg\\VSTPlugins");
      ret.add ("C:\\Program Files\\Common Files\\VST2");
      ret.add ("C:\\Program Files\\Common Files\\Steinberg\\VST2");
#elif defined(__APPLE__)
      ret.add ("/Library/Audio/Plug-ins/VST");
#elif defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/vst");
#else /* non-flatpak UNIX */
      {
        auto home_vst = fs::path (Glib::get_home_dir ()) / ".vst";
        ret.add (home_vst);
      }
      ret.add ("/usr/lib/vst");
      ret.add ("/usr/local/lib/vst");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/vst");
      ret.add ("/usr/local/lib64/vst");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/vst");
          ret.add ("/usr/local/" LIBDIR_NAME "/vst");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("VST2 paths");

  return ret;
}

StringArray
PluginManager::get_vst3_paths ()
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${VST3_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("VST3 paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_vst3_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret.add ("C:\\Program Files\\Common Files\\VST3");
#elif defined(__APPLE__)
      ret.add ("/Library/Audio/Plug-ins/VST3");
#elif defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/vst3");
#else /* non-flatpak UNIX */
      {
        auto home_vst3 = fs::path (Glib::get_home_dir ()) / ".vst3";
        ret.add (home_vst3);
      }
      ret.add ("/usr/lib/vst3");
      ret.add ("/usr/local/lib/vst3");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/vst3");
      ret.add ("/usr/local/lib64/vst3");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/vst3");
          ret.add ("/usr/local/" LIBDIR_NAME "/vst3");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("VST3 paths");

  return ret;
}

StringArray
PluginManager::get_sf_paths (bool sf2)
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      ret.add (G_SEARCHPATH_SEPARATOR_S);
      return ret;
    }

  auto paths_from_settings =
    sf2 ? gui::SettingsManager::get_instance ()->get_sf2_search_paths ()
        : gui::SettingsManager::get_instance ()->get_sfz_search_paths ();
  add_expanded_paths (ret, paths_from_settings);

  return ret;
}

StringArray
PluginManager::get_dssi_paths ()
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${DSSI_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("DSSI paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_dssi_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#if defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/dssi");
#else /* non-flatpak UNIX */
      {
        auto home_dssi = fs::path (Glib::get_home_dir ()) / ".dssi";
        ret.add (home_dssi.string ());
      }
      ret.add ("/usr/lib/dssi");
      ret.add ("/usr/local/lib/dssi");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/dssi");
      ret.add ("/usr/local/lib64/dssi");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/dssi");
          ret.add ("/usr/local/" LIBDIR_NAME "/dssi");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("DSSI paths");

  return ret;
}

StringArray
PluginManager::get_ladspa_paths ()
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${LADSPA_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("LADSPA paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_ladspa_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#if defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/ladspa");
#else /* non-flatpak UNIX */
      ret.add ("/usr/lib/ladspa");
      ret.add ("/usr/local/lib/ladspa");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/ladspa");
      ret.add ("/usr/local/lib64/ladspa");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/ladspa");
          ret.add ("/usr/local/" LIBDIR_NAME "/ladspa");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("LADSPA paths");

  return ret;
}

StringArray
PluginManager::get_clap_paths ()
{
  auto ret = StringArray ();

#ifndef CARLA_HAVE_CLAP_SUPPORT
  return ret;
#endif

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${CLAP_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("CLAP paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_clap_search_paths ();
  if (paths_from_settings.empty ())
    {
      /* no paths given - use default */
#ifdef _WIN32
      ret.add ("C:\\Program Files\\Common Files\\CLAP");
      ret.add ("C:\\Program Files (x86)\\Common Files\\CLAP");
#elif defined(__APPLE__)
      ret.add ("/Library/Audio/Plug-ins/CLAP");
#elif defined(FLATPAK_BUILD)
      ret.add ("/app/extensions/Plugins/clap");
#else /* non-flatpak UNIX */
      {
        auto home_clap = fs::path (Glib::get_home_dir ()) / ".clap";
        ret.add (home_clap);
      }
      ret.add ("/usr/lib/clap");
      ret.add ("/usr/local/lib/clap");
#  if ZRYTHM_IS_INSTALLER_VER
      ret.add ("/usr/lib64/clap");
      ret.add ("/usr/local/lib64/clap");
#  else  /* else if unix and not installer ver */
      if (std::string (LIBDIR_NAME) != "lib")
        {
          ret.add ("/usr/" LIBDIR_NAME "/clap");
          ret.add ("/usr/local/" LIBDIR_NAME "/clap");
        }
#  endif /* endif non-flatpak UNIX */
#endif   /* endif per-platform code */
    }
  else
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("CLAP paths");

  return ret;
}

StringArray
PluginManager::get_jsfx_paths ()
{
  auto ret = StringArray ();

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      QStringList paths_from_settings = { "${JSFX_PATH}" };
      add_expanded_paths (ret, paths_from_settings);

      ret.print ("JSFX paths");
      return ret;
    }

  auto paths_from_settings =
    gui::SettingsManager::get_instance ()->get_jsfx_search_paths ();
  if (!paths_from_settings.empty ())
    {
      /* use paths given */
      add_expanded_paths (ret, paths_from_settings);
    }

  ret.print ("JSFX paths");

  return ret;
}

StringArray
PluginManager::get_au_paths ()
{
  auto ret = StringArray ();

  ret.add ("/Library/Audio/Plug-ins/Components");
  auto user_components =
    fs::path (Glib::get_home_dir ()) / "Library" / "Audio" / "Plug-ins"
    / "Components";
  ret.add (user_components.string ());

  ret.print ("AU paths");

  return ret;
}

bool
PluginManager::supports_protocol (PluginProtocol protocol)
{
  // List of protocols that are always supported
  const auto always_supported = {
    PluginProtocol::DUMMY, PluginProtocol::LV2,  PluginProtocol::LADSPA,
    PluginProtocol::VST,   PluginProtocol::VST3, PluginProtocol::SFZ,
    PluginProtocol::JSFX,  PluginProtocol::CLAP
  };

  // Check if the protocol is in the always supported list
  if (
    std::ranges::find (always_supported, protocol)
    != std::ranges::end (always_supported))
    {
#if HAVE_CARLA
      return true;
#else
      return false;
#endif
    }

#if HAVE_CARLA
  // Get the list of features supported by Carla
  const StringArray carla_supported_features (carla_get_supported_features ());

  // Map of protocols to their corresponding Carla feature names
  const auto feature_map = std::map<PluginProtocol, std::string>{
    { PluginProtocol::SF2,  "sf2"  },
    { PluginProtocol::DSSI, "osc"  },
    { PluginProtocol::VST3, "vst3" },
    { PluginProtocol::AU,   "au"   }
  };

  // Check if the protocol is in the feature map
  if (auto it = feature_map.find (protocol); it != feature_map.end ())
    {
      // Check if the corresponding feature is supported by Carla
      return std::ranges::any_of (
        carla_supported_features, [&] (const auto &feature) {
          return feature.toStdString () == it->second;
        });
    }
#endif

  // If not found in any of the above cases, return false
  return false;
}

std::vector<fs::path>
PluginManager::get_paths_for_protocol (const PluginProtocol protocol)
{
  StringArray paths;
  switch (protocol)
    {
    case PluginProtocol::VST:
      paths = get_vst2_paths ();
      break;
    case PluginProtocol::VST3:
      paths = get_vst3_paths ();
      break;
    case PluginProtocol::DSSI:
      paths = get_dssi_paths ();
      break;
    case PluginProtocol::LADSPA:
      paths = get_ladspa_paths ();
      break;
    case PluginProtocol::SFZ:
      paths = get_sf_paths (false);
      break;
    case PluginProtocol::SF2:
      paths = get_sf_paths (true);
      break;
    case PluginProtocol::CLAP:
      paths = get_clap_paths ();
      break;
    case PluginProtocol::JSFX:
      paths = get_jsfx_paths ();
      break;
    case PluginProtocol::LV2:
      paths = get_lv2_paths ();
      break;
    case PluginProtocol::AU:
      paths = get_au_paths ();
      break;
    default:
      break;
    }

  std::vector<fs::path> ret;
  for (const auto &path : paths)
    {
      ret.push_back (path.toStdString ());
    }

  return ret;
}

std::string
PluginManager::get_paths_for_protocol_separated (const PluginProtocol protocol)
{
  auto paths = get_paths_for_protocol (protocol);
  if (!paths.empty ())
    {
      auto path_strings = std::vector<std::string> ();
      std::ranges::transform (
        paths, std::back_inserter (path_strings),
        [] (const auto &path) { return path.string (); });
      auto paths_separated =
        string_join (path_strings, G_SEARCHPATH_SEPARATOR_S);
      return paths_separated;
    }
  else
    {
      return "";
    }
}

fs::path
PluginManager::find_plugin_from_rel_path (
  const PluginProtocol   protocol,
  const std::string_view rel_path) const
{
  auto paths = get_paths_for_protocol (protocol);
  if (paths.empty ())
    {
      z_warning ("no paths for {}", rel_path.data ());
      return {};
    }

  for (const auto &path : paths)
    {
      auto full_path = fs::path (path) / rel_path;
      if (fs::exists (full_path))
        {
          return full_path;
        }
    }

  z_warning ("no paths for {}", rel_path);
  return {};
}

void
PluginManager::add_descriptor (const zrythm::plugins::PluginDescriptor &descr)
{
  z_return_if_fail (descr.protocol_ > PluginProtocol::DUMMY);
  plugin_descriptors_->addDescriptor (descr);
  add_category_and_author (descr.category_str_, descr.author_);
}

void
PluginManager::call_carla_discovery_idle ()
{
#if HAVE_CARLA
  bool done = carla_discovery_->idle ();
  if (done)
    {
      /* sort alphabetically */
      plugin_descriptors_->sort (0, Qt::SortOrder::AscendingOrder);
      // std::sort (
      // plugin_descriptors_.begin (), plugin_descriptors_.end (),
      // [] (const auto &a, const auto &b) { return a.name_ < b.name_; });
      std::sort (plugin_categories_.begin (), plugin_categories_.end ());
      std::sort (plugin_authors_.begin (), plugin_authors_.end ());

      cached_plugin_descriptors_->serialize_to_file_no_throw ();

      // Stop the timer
      auto * timer = qobject_cast<QTimer *> (sender ());
      if (timer != nullptr)
        timer->stop ();

      Q_EMIT scanFinished ();
    }
#endif // HAVE_CARLA
}

void
PluginManager::beginScan ()
{
  if (qEnvironmentVariableIsSet ("ZRYTHM_SKIP_PLUGIN_SCAN"))
    {
      Q_EMIT scanFinished ();
      return;
    }

  QObject::connect (
    scanner_.get (), &PluginScanner::currentlyScanningPluginChanged, this,
    &PluginManager::currentlyScanningPluginChanged);
  scanner_->beginScan ();

#if 0
  for (
    size_t i = ENUM_VALUE_TO_INT (PluginProtocol::LV2);
    i <= ENUM_VALUE_TO_INT (PluginProtocol::JSFX); i++)
    {
      PluginProtocol cur = ENUM_INT_TO_VALUE (PluginProtocol, i);
      if (!supports_protocol (cur))
        continue;

      carla_discovery_->start (CarlaBackend::BINARY_NATIVE, cur);
      /* also scan 32-bit on windows */
#  ifdef _WIN32
      carla_discovery_->start (CarlaBackend::BINARY_WIN32, cur);
#  endif
    }

  // Connect to the event loop using QTimer
  auto * timer = new QTimer (this);
  connect (
    timer, &QTimer::timeout, this, &PluginManager::call_carla_discovery_idle);
  timer->start (
    0); // 0 ms interval means it will be called on every event loop iteration
#endif
}

std::unique_ptr<PluginDescriptor>
PluginManager::find_plugin_from_uri (std::string_view uri) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&uri] (const zrythm::plugins::PluginDescriptor &descr) {
      return uri == descr.uri_;
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for URI {} not found", uri);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<PluginDescriptor>
PluginManager::find_from_descriptor (
  const zrythm::plugins::PluginDescriptor &src_descr) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&src_descr] (const zrythm::plugins::PluginDescriptor &descr) {
      return src_descr.is_same_plugin (descr);
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for {} not found", src_descr.name_);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<PluginDescriptor>
PluginManager::pick_instrument () const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [] (const auto &descr) { return descr.is_instrument (); });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("no instrument found");
      return nullptr;
    }
#endif
  return nullptr;
}

void
PluginManager::clear_plugins ()
{
  plugin_descriptors_->clear ();
  plugin_categories_.clear ();
  plugin_authors_.clear ();
}