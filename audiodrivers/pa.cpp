//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2002-2010 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "pa.h"

#include <portaudio.h>

#ifdef USE_ALSA
#include "alsa.h"
#include "alsamidi.h"
#endif

#include "mididriver.h"

#include "libmscore/instrument.h"
#include "libmscore/score.h"

#include "mscore/preferences.h"
#include "mscore/seq.h"

#ifdef USE_PORTMIDI
#include "pm.h"
#endif

namespace Ms {

static PaStream* stream;

//---------------------------------------------------------
//   paCallback
//---------------------------------------------------------

int paCallback(const void*, void* out, long unsigned frames,
   const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void *)
      {
      seq->setInitialMillisecondTimestampWithLatency();
      seq->process((unsigned)frames, (float*)out);
      return 0;
      }

//---------------------------------------------------------
//   Portaudio
//---------------------------------------------------------

Portaudio::Portaudio(Seq* s)
   : Driver(s)
      {
      _sampleRate = 48000;    // will be replaced by device default sample rate
      initialized = false;
      state       = Transport::STOP;
      seekflag    = false;
      pos = 0;
      startTime = 0.0;
      midiDriver  = 0;
      }

//---------------------------------------------------------
//   ~Portaudio
//---------------------------------------------------------

Portaudio::~Portaudio()
      {
      if (initialized) {
            PaError err = Pa_CloseStream(stream);
            if (err != paNoError)
                  qDebug("Portaudio close stream failed: %s", Pa_GetErrorText(err));
            Pa_Terminate();
            if (midiDriver)
                  delete midiDriver;
            }
      }

//---------------------------------------------------------
//   init
//    return false on error
//---------------------------------------------------------

bool Portaudio::init(bool)
      {
      PaError err = Pa_Initialize();
      if (err != paNoError) {
            qDebug("Portaudio initialize failed: %s", Pa_GetErrorText(err));
            return false;
            }
      initialized = true;
      if (MScore::debugMode)
            qDebug("using PortAudio Version: %s", Pa_GetVersionText());

      PaDeviceIndex idx = preferences.getInt(PREF_IO_PORTAUDIO_DEVICE);
      if (idx < 0) {
            idx = Pa_GetDefaultOutputDevice();
            qDebug("No device selected.  PortAudio detected %d devices.  Will use the default device (index %d).", Pa_GetDeviceCount(), idx);
            }

      const PaDeviceInfo* di = Pa_GetDeviceInfo(idx);

      //select default output device if no device or device without output channels have been selected
      if (di == nullptr || di->maxOutputChannels < 1)
            di = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice());

      if (!di)
            return false;    // Portaudio is not properly initialized; disable audio
      _sampleRate = int(di->defaultSampleRate);

      /* Open an audio I/O stream. */
      struct PaStreamParameters out;
      memset(&out, 0, sizeof(out));

      out.device           = idx;
      out.channelCount     = 2;
      out.sampleFormat     = paFloat32;
      out.suggestedLatency = di->defaultLowOutputLatency;
      out.hostApiSpecificStreamInfo = 0;

      err = Pa_OpenStream(&stream, 0, &out, double(_sampleRate), 0, 0, paCallback, (void*)this);
      if (err != paNoError) {
            // fall back to default device:
            out.device = Pa_GetDefaultOutputDevice();
            err = Pa_OpenStream(&stream, 0, &out, double(_sampleRate), 0, 0, paCallback, (void*)this);
            if (err != paNoError) {
                  qDebug("Portaudio open stream %d failed: %s", idx, Pa_GetErrorText(err));
                  return false;
                  }
            }
      const PaStreamInfo* si = Pa_GetStreamInfo(stream);
      if (si)
            _sampleRate = int(si->sampleRate);
#ifdef USE_ALSA
      midiDriver = new AlsaMidiDriver(seq);
#endif
#ifdef USE_PORTMIDI
      midiDriver = new PortMidiDriver(seq);
#endif
      if (midiDriver && !midiDriver->init()) {
            qDebug("Init midi driver failed");
            delete midiDriver;
            midiDriver = 0;
#ifdef USE_PORTMIDI
            return true;                  // return OK for audio driver; midi is only input
#else
            return false;
#endif
            }
      return true;
      }

//---------------------------------------------------------
//   apiList
//---------------------------------------------------------

QStringList Portaudio::apiList() const
      {
      QStringList al;

      PaHostApiIndex apis = Pa_GetHostApiCount();
      for (PaHostApiIndex i = 0; i < apis; ++i) {
            const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
            if (info)
                  al.append(QString::fromLocal8Bit(info->name));
            }
      return al;
      }

//---------------------------------------------------------
//   deviceList
//---------------------------------------------------------

QStringList Portaudio::deviceList(int apiIdx)
      {
      QStringList dl;
      const PaHostApiInfo* info = Pa_GetHostApiInfo(apiIdx);
      if (info) {
            for (int i = 0; i < info->deviceCount; ++i) {
                  PaDeviceIndex idx = Pa_HostApiDeviceIndexToDeviceIndex(apiIdx, i);
                  const PaDeviceInfo* di = Pa_GetDeviceInfo(idx);
                  if (di)
                        dl.append(QString::fromLocal8Bit(di->name));
                  }
            }
      return dl;
      }

//---------------------------------------------------------
//   deviceIndex
//---------------------------------------------------------

int Portaudio::deviceIndex(int apiIdx, int apiDevIdx)
      {
      return Pa_HostApiDeviceIndexToDeviceIndex(apiIdx, apiDevIdx);
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

bool Portaudio::start(bool)
      {
      PaError err = Pa_StartStream(stream);
      if (err != paNoError) {
            qDebug("Portaudio: start stream failed: %s", Pa_GetErrorText(err));
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

bool Portaudio::stop()
      {
      PaError err = Pa_StopStream(stream);      // sometimes the program hangs here on exit
      if (err != paNoError) {
            qDebug("Portaudio: stop failed: %s", Pa_GetErrorText(err));
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

int Portaudio::framePos() const
      {
      return 0;
      }

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void Portaudio::startTransport()
      {
      state = Transport::PLAY;
      }

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void Portaudio::stopTransport()
      {
      state = Transport::STOP;
      }

//---------------------------------------------------------
//   getState
//---------------------------------------------------------

Transport Portaudio::getState()
      {
      return state;
      }

//---------------------------------------------------------
//   midiRead
//---------------------------------------------------------

void Portaudio::midiRead()
      {
      if (midiDriver)
            midiDriver->read();
      }

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

#ifdef USE_PORTMIDI

// Prevent killing sequencer with wrong data
#define less128(__less) ((__less >=0 && __less <= 127) ? __less : 0)

//---------------------------------------------------------
//   outputChannel
//    The one PortMidi output has 16 channels and no ports. A score's own
//    channels can collide there: a MusicXML file may fix some parts' channels
//    while the others are numbered from 1, and a second port folds onto the first.
//    Every part channel gets its own MIDI channel instead, in score order,
//    with drum sets (bank 128, or channel 10 in the score) on channel 10.
//---------------------------------------------------------

static int outputChannel(MasterScore* score, int idx)
      {
      const std::vector<MidiMapping>& mapping = score->midiMapping();
      int next = 0;
      for (int i = 0; i < int(mapping.size()); ++i) {
            const MidiMapping& m = mapping[i];
            const bool drums = m.channel() == 9 || (m.articulation() && m.articulation()->bank() == 128);
            if (i == idx)
                  return drums ? 9 : next;
            if (!drums && ++next == 9)
                  ++next;
            next %= 16;
            }
      return score->midiChannel(idx);
      }

// TODO: this was copied from Jack version...I'd like to eventually unify these two, so that they handle midi event types in the same manner
void Portaudio::putEvent(const NPlayEvent& e, unsigned framePos)
      {
      PortMidiDriver* portMidiDriver = static_cast<PortMidiDriver*>(midiDriver);
      if (!portMidiDriver || !portMidiDriver->getOutputStream() || !portMidiDriver->canOutput())
            return;

      int portIdx = seq->score()->midiPort(e.channel());
      int chan    = outputChannel(seq->score(), e.channel());

      if (portIdx < 0 ) {
            qDebug("Portaudio::putEvent: invalid port %d", portIdx);
            return;
            }

      if (midiOutputTrace) {
            int a     = e.dataA();
            int b     = e.dataB();
            qDebug("MidiOut<%d>: Portaudio: %02x %02x %02x, chan: %i", portIdx, e.type(), a, b, chan);
            }

      switch(e.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_CONTROLLER:
                  // Catch CTRL_PROGRAM and let other ME_CONTROLLER events to go
                  if (e.dataA() == CTRL_PROGRAM) {
                        // Convert CTRL_PROGRAM event to ME_PROGRAM
                        int msg = Pm_Message(ME_PROGRAM | chan, less128(e.dataB()), 0);
                        PmError error = Pm_WriteShort(portMidiDriver->getOutputStream(), seq->getCurrentMillisecondTimestampWithLatency(framePos), msg);
                        if (error != pmNoError) {
                              qDebug("Portaudio: error %d", error);
                              return;
                              }
                        break;
                        }
                  // fall through
            case ME_PITCHBEND:
                  {
                  int dataB = e.dataB();
                  // A drum set is bank 128 in the internal synth, bank MSB 1 on the wire;
                  // an XG module takes a drum kit from MSB 127 and would play MSB 1 as a melodic voice
                  if (e.type() == ME_CONTROLLER && e.dataA() == CTRL_HBANK && dataB == 1)
                        dataB = 127;
                  int msg = Pm_Message(e.type() | chan, less128(e.dataA()), less128(dataB));
                  PmError error = Pm_WriteShort(portMidiDriver->getOutputStream(), seq->getCurrentMillisecondTimestampWithLatency(framePos), msg);
                  if (error != pmNoError) {
                        qDebug("Portaudio: error %d", error);
                        return;
                        }
                  }
                  break;

            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  {
                  int msg = Pm_Message(e.type() | chan, less128(e.dataA()), 0);
                  PmError error = Pm_WriteShort(portMidiDriver->getOutputStream(), seq->getCurrentMillisecondTimestampWithLatency(framePos), msg);
                  if (error != pmNoError) {
                        qDebug("Portaudio: error %d", error);
                        return;
                        }
                  }
                  break;
            case ME_SONGPOS:
            case ME_CLOCK:
            case ME_START:
            case ME_CONTINUE:
            case ME_STOP:
                  qDebug("Portaudio: event type %x not supported", e.type());
                  break;
            }
      }
#endif

//---------------------------------------------------------
//   currentApi
//---------------------------------------------------------

int Portaudio::currentApi() const
      {
      PaDeviceIndex idx = preferences.getInt(PREF_IO_PORTAUDIO_DEVICE);
      if (idx < 0)
            idx = Pa_GetDefaultOutputDevice();

      for (int api = 0; api < Pa_GetHostApiCount(); ++api) {
            const PaHostApiInfo* info = Pa_GetHostApiInfo(api);
            if (info) {
                  for (int k = 0; k < info->deviceCount; ++k) {
                        PaDeviceIndex i = Pa_HostApiDeviceIndexToDeviceIndex(api, k);
                        if (i == idx)
                              return api;
                        }
                  }
            }
      qDebug("Portaudio: no current api found for device %d", idx);
      return -1;
      }

//---------------------------------------------------------
//   currentDevice
//---------------------------------------------------------

int Portaudio::currentDevice() const
      {
      PaDeviceIndex idx = preferences.getInt(PREF_IO_PORTAUDIO_DEVICE);
      if (idx < 0)
            idx = Pa_GetDefaultOutputDevice();

      for (int api = 0; api < Pa_GetHostApiCount(); ++api) {
            const PaHostApiInfo* info = Pa_GetHostApiInfo(api);
            if (info) {
                  for (int k = 0; k < info->deviceCount; ++k) {
                        PaDeviceIndex i = Pa_HostApiDeviceIndexToDeviceIndex(api, k);
                        if (i == idx)
                              return k;
                        }
                  }
            }
      qDebug("Portaudio: no current ApiDevice found for device %d", idx);
      return -1;
      }
}

