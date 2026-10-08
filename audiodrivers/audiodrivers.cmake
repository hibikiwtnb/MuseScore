# The audio drivers drive the sequencer and the main window (seq->..., mscore->...),
# so they cannot live in the audio target without making audio depend on the
# application. They are compiled into mscoreapp instead, and kept here rather
# than under audio/ to make that distinction visible.
set (AUDIODRIVERS_DIR ${CMAKE_CURRENT_LIST_DIR})

set (AUDIODRIVERS_SRC
    ${AUDIODRIVERS_DIR}/driver.h
    ${AUDIODRIVERS_DIR}/driver.cpp
    )

if ( NOT MINGW AND NOT MSVC )
    if (USE_ALSA)
          set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/alsa.cpp ${AUDIODRIVERS_DIR}/alsa.h ${AUDIODRIVERS_DIR}/alsamidi.h)
    endif (USE_ALSA)
endif ( NOT MINGW AND NOT MSVC )

if (USE_PORTAUDIO)
    set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/pa.cpp ${AUDIODRIVERS_DIR}/pa.h)
endif (USE_PORTAUDIO)

if (USE_PULSEAUDIO)
    set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/pulseaudio.cpp)
endif (USE_PULSEAUDIO)

if (USE_PORTMIDI)
    set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/pm.cpp ${AUDIODRIVERS_DIR}/pm.h)
endif (USE_PORTMIDI)

if (USE_JACK)
      set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/jackaudio.cpp ${AUDIODRIVERS_DIR}/jackweakapi.cpp ${AUDIODRIVERS_DIR}/jackaudio.h)
endif (USE_JACK)

if (USE_ALSA OR USE_PORTMIDI)
      set (AUDIODRIVERS_SRC ${AUDIODRIVERS_SRC} ${AUDIODRIVERS_DIR}/mididriver.cpp ${AUDIODRIVERS_DIR}/mididriver.h)
endif (USE_ALSA OR USE_PORTMIDI)


