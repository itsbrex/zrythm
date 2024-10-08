# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(blp_filenames
  ui/chord_selector_window.blp
  ui/project_scale_popover.blp
  ui/audio_editor_space.blp
  ui/automation_editor_space.blp
  ui/active_hardware_popover.blp
  ui/bug_report_dialog.blp
  ui/main_window.blp
  ui/cc-list-row-info-button.blp
  ui/chord_editor_space.blp
  ui/fader_buttons.blp
  ui/snap_box.blp
  ui/snap_grid_popover.blp
  ui/timeline_panel.blp
  ui/folder_channel.blp
  ui/transport_controls.blp
  ui/event_viewer.blp
  ui/expander_box.blp
  ui/editor_toolbar.blp
  ui/project_toolbar.blp
  ui/fishbowl_window.blp
  ui/scale_selector_window.blp
  ui/bot_bar.blp
  ui/inspector_track.blp
  ui/chord_pad_panel.blp
  ui/center_dock.blp
  ui/automatable_selector.blp
  ui/string_entry_dialog.blp
  ui/range_action_buttons.blp
  ui/right_dock_edge.blp
  ui/timeline_toolbar.blp
  ui/monitor_section.blp
  ui/modulator_view.blp
  ui/log_viewer.blp
  ui/quantize_box.blp
  ui/track.blp
  ui/chord_key.blp
  ui/control_room.blp
  ui/fader_controls_grid.blp
  ui/playhead_scroll_buttons.blp
  ui/clip_editor.blp
  ui/left_dock_edge.blp
  ui/zoom_buttons.blp
  ui/bind_cc_dialog.blp
  ui/clip_editor_inner.blp
  ui/modulator_macro.blp
  ui/tracklist_header.blp
  ui/toolbox.blp
  ui/greeter.blp
  ui/file_auditioner_controls.blp
  ui/mixer.blp
  ui/channel.blp
  ui/panel_file_browser.blp
  ui/plugin_browser.blp
  ui/quantize_dialog.blp
  ui/inspector_plugin.blp
  ui/modulator_inner.blp
  ui/export_dialog.blp
  ui/port_selector_dialog.blp
  ui/midi_editor_space.blp
  ui/bounce_dialog.blp
  ui/chord_pack_browser.blp
  ui/generic_progress_dialog.blp
  ui/file_browser_filters.blp
  gtk/menus.blp
  gtk/help-overlay.blp
)

set(ui_files)
foreach(blp ${blp_filenames})
  string(REPLACE ".blp" ".ui" ui_file ${blp})
  list(APPEND ui_files ${ui_file})
endforeach()

# GL shaders
# file(GLOB gl_shaders "${CMAKE_CURRENT_SOURCE_DIR}/gl/shaders/*.glsl")
