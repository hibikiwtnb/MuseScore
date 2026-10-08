#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2013 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(MODULE audio)

include (${CMAKE_CURRENT_LIST_DIR}/midi/midi.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/exports/exports.cmake)

QT5_WRAP_UI (ui_headers
    ${MIDI_UI}
    )

add_library (${MODULE} STATIC
      ${ui_headers}
      ${MIDI_SRC}
      ${EXPORTS_SRC}
      )

target_include_directories(${MODULE} 
   PRIVATE 
      ${PROJECT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_LIST_DIR}
      ${PROJECT_SOURCE_DIR}/global
      ${PROJECT_SOURCE_DIR}/thirdparty  # because include "mscore/musescore.h"
   )

# Do not link mscoreapp here: mscoreapp links audio, and the resulting static
# library cycle makes AUTOMOC unusable with the Xcode generator (CMake >= 4.4).
# audio's sources include mscore/*.h, but those symbols are resolved when the
# app or a test binary links mscoreapp itself.
target_link_libraries (${MODULE}
      ${QT_LIBRARIES}
      audiofile
      libmscore   # for libmscore/*.h and the freetype include dirs it carries
      preferences
      icons       # for the fluid and zerberus settings GUIs
      extension   # to find the soundfonts and sfzs extensions ship
      )

if (BUILD_PCH)
    target_use_pch(${MODULE})
endif (BUILD_PCH)
