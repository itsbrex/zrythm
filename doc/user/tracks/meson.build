# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'automation.rst',
  'inspector-page.rst',
  'intro.rst',
  'track-types.rst',
  'creating-tracks.rst',
  'overview.rst',
  'track-lanes.rst',
  'track-operations.rst',
  'tracklist.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
