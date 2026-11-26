# MuseScore-Bandscore (3.8 Bandscore)

A fork of MuseScore 3 for opening, checking and playing band scores read by OMR with [Audiveris-Bandscore](https://github.com/hibikiwtnb/audiveris-bandscore). It is based on [MuseScore 3.7 Evolution](https://github.com/Jojo-Schmitz/MuseScore) maintained by Jojo-Schmitz; the version is named **3.8 Bandscore**.

Japanese band scores write things that official MuseScore cannot show or play. The most basic one is **2x**: notes in brackets that are played only on the second pass, inside the same measure. Official MuseScore has no such notation, so it can neither display nor play it. This fork adds it, along with the other repeat structures these scores use.

中文說明見 [Audiveris-Bandscore](https://github.com/hibikiwtnb/audiveris-bandscore)（兩個專案一起使用）。

## Download

Prebuilt versions are on the [Releases](https://github.com/hibikiwtnb/MuseScore/releases) page:

- macOS: Apple Silicon and Intel `.dmg`. The app is not notarized: the first time, right-click it in Applications and choose Open, or run `xattr -dr com.apple.quarantine "/Applications/MuseScore 3.8 Bandscore.app"`.
- Windows 64-bit: a portable `.paf.exe` and a `.7z` archive (run `bin\MuseScore3Evo.exe`).

The macOS app has its own name and bundle ID, and its settings are kept apart from official MuseScore 3, so both can be installed side by side.

## What is different

**Repeats and passes**
- **2x measures**: the first and second pass of a measure are shown side by side in the same measure, the second inside "2x ( … )", without adding measures; on repeat each pass plays only its own notes (MusicXML `time-only`).
- **Notes played on one pass only**: notes marked for certain passes (e.g. "1x tacet") play by pass number; repeats, D.S. and D.C. each count as a pass.
- **Numbered segnos and codas**: D.S.1 jumps to segno1, To Coda to coda1, so multi-part jumps follow the printed score.
- **Measure repeats of 1, 2 and 4 measures**: imported from MusicXML, shown as repeat signs and played as repeats (MuseScore 3 ignored them on MusicXML import).

**MusicXML import and display**
- Every part's staves are shown in every system; official MuseScore hides resting parts, so each system shows different instruments.
- Chord symbols are not played by default, so misread chords do not sound wrong (can be turned on in the style settings).
- The hi-hat open sign (`<open/>`, MusicXML 4.0) is read.
- Down-tuned guitars and basses keep the written spelling of the sounding pitch (A♭, not G♯).
- The printed label of an instrument change becomes the visible instrument change, at the right measure.
- Hundreds of meaningless import errors per score (doubled ties, valid clefs reported as invalid) are gone.

**Layout**
- **Equal measure widths**: every measure in a system gets the same width for its notes, however dense; clefs, key and time signatures at the start of a line or measure are not counted. A measure whose notes need more room keeps its minimum width and the others share the rest equally. Measure stretch (`{` `}`) is ignored while the option is on; the values stay in the score and apply again when it is off. On by default, also for existing scores; turn it off in Format > Style > Measure ("Equal measure widths within a system").

**Playback**
- **External MIDI devices**: a checkbox in the I/O preferences chooses the MIDI output device or MuseScore's own sounds. Each part channel takes its own MIDI channel when it first plays, drums on channel 10, melodic parts never on 10, and XG part mode is set to Normal, so XG sound modules no longer play melodies as drums. `-O` traces the channels and programs sent.
- **Several soundfonts**: a program the first soundfont lacks is taken from the next soundfont that has it, instead of falling back to piano.

**Video View**
- A view mode for recording score videos: one page at a time in a 16:9 frame, with a piano keyboard along the bottom which lights the sounding keys during playback. The keyboard covers the whole octaves the score uses. The wheel and the page keys turn pages. It does not change the printable page.

## Build

Same as MuseScore 3.x (see "Building" below). On macOS this fork also builds with Ninja or Makefiles, not only Xcode. The CI workflow "Build: All" runs by hand only and by default builds macOS (Apple, Intel) and Windows x64.

## License

GPL version 2.0, as upstream.

---

# Upstream README (MuseScore 3.x)

![MuseScore](mscore/data/musescore_logo_full.png)  
Music notation and composition software

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)

MuseScore is an open source and free music notation software. For support, contribution, and bug reports visit MuseScore.org. Fork and make pull requests!

## Features

- WYSIWYG design, notes are entered on a "virtual notepaper"
- TrueType font(s) for printing & display allows for high quality scaling to all sizes
- Easy & fast note entry
- Many editing functions
- MusicXML import/export
- MIDI (SMF) import/export
- MuseData import
- MIDI input for note entry
- Integrated sequencer and software synthesizer to play the score
- Print or create PDF files

## More info
- [MuseScore Homepage](https://musescore.org)
- [MuseScore Git workflow instructions](https://musescore.org/en/developers-handbook/git-workflow)
- [How to compile MuseScore?](https://musescore.org/en/developers-handbook/compilation)

## License
MuseScore is licensed under GPL version 2.0. See [LICENSE.GPL](https://github.com/musescore/MuseScore/blob/master/LICENSE.GPL) in the same directory.

## Packages
- **aeolus:** Clone of [Aeolus](http://kokkinizita.linuxaudio.org/linuxaudio/aeolus/). Disabled by default in the stable releases. See http://dev-list.musescore.org/Aeolus-Organ-Synth-td7578364.html. Kept as an example of how to integrate with a complex synthesizer.

- **assets:** Graphical assets, use them if you need a MuseScore icon. For logo, color, etc., see https://musescore.org/en/about/logos-and-graphics.

- **awl:** Audio Widget Library, from the MusE project.

- **build:** Utility files for build.

- **bww2mxml:** Command line tool to convert BWW files to MusicXML. BWW parser is used by MuseScore to import BWW files.

- **demos:** A few MuseScore files to demonstrate what can be done.

- **fluid:** Clone of [FluidSynth@sourceforge](https://sourceforge.net/projects/fluidsynth), ported to C++ and customized. Code now at [fluidsynth@github](https://github.com/FluidSynth/fluidsynth).

- **fonts:** Contains FontForge source (sfd) + ttf/otf fonts. MuseScore includes the "Emmentaler" font from the LilyPond project.

- **libmscore:** Data model of MuseScore.

- **mscore:** Main code for the MuseScore UI.

- **msynth:** Abstract interface to Fluid + Aeolus.

- **mtest:** Unit testing using QTest.

- **omr:** Optical music recognition.

- **share:** Files moved to /usr/share/... on install.

- **test:** Old tests. Should move to mtest.

- **vtest:** Visual tests. Compare reference images with current implementation.

- **thirdparty:** Contains projects which are included for convenience, usually to integrate them into the build system to make them available for all supported platforms.

    - **thirdparty/rtf2html:**
    Used for capella import. Clone from [rtf2html@sourceforge](https://sourceforge.net/projects/rtf2html), code now at [rtf2html@github](https://github.com/lvu/rtf2html).

    - **thirdparty/dtl:**
    Used for the score comparison tool. [Diff Template Library](https://github.com/cubicdaiya/dtl).

    - **thirdparty/ofqf:**
    OSC server interface. Based on [OSC for Qt4](http://www.arnoldarts.de/projects/ofqf/), code now at [ofq@github](https://github.com/kampfschlaefer/ofq).

    - **thirdparty/singleapp:**
    Clone from [Qt Single Application](https://github.com/qtproject/qt-solutions/tree/master/qtsingleapplication).

    - **thirdparty/portmidi:**
    Clone from [PortMidi](https://sourceforge.net/projects/portmedia/).

    - **thirdparty/beatroot:**
    It's a core part of [BeatRoot Vamp Plugin](https://code.soundsoftware.ac.uk/projects/beatroot-vamp/repository) by Simon Dixon and Chris Cannam, used in MIDI import for beat detection.

    - **thirdparty/qt-google-analytics:**
    Clone from [qt-google-analytics](https://github.com/HSAnet/qt-google-analytics).


## Building
**Read the developer handbook for a [complete build walkthrough](https://musescore.org/en/developers-handbook/compilation) and a list of dependencies.**

### Getting sources
If using git to download repo of entire code history, type:

    git clone https://github.com/musescore/MuseScore.git
    cd MuseScore

Otherwise, you can just download the latest source release tarball from the [Releases page](https://github.com/musescore/MuseScore/releases), and then from your download directory type:

    tar xzf MuseScore-x.x.x.tar.gz
    cd MuseScore-x.x.x

### Release Build
To compile MuseScore, type:

    make release

If something goes wrong, then remove the whole build subdirectory with `make clean` and start new with `make release`.

### Running
To start MuseScore, type:

    ./build.release/mscore/mscore

The Start Center window will appear on every invocation until you disable that setting via the "Preferences" dialog.

### Installing
To install to default prefix using root user, type:

    sudo make install

### Debug Build
A debug version can be built by doing `make debug` instead of `make release`.

To run the debug version, type:

    ./build.debug/mscore/mscore

### Testing
See [mtest/README.md](/mtest/README.md) or [the developer handbook](https://musescore.org/handbook/developers-handbook/finding-your-way-around/automated-tests) for instructions on how to run the test suite.

The new [script testing facility](https://musescore.org/node/278278) is also available to create your own automated tests. Please try it out!

### Getting ready-made packages for MuseScore 3.7 Evolution and instructions how to 'install'
See the [Wiki](https://github.com/Jojo-Schmitz/MuseScore/wiki).
