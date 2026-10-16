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
//   OutputChannels
//    The one PortMidi output has 16 channels and no ports, while a score can
//    use more: instrument changes, and articulations a part never plays
//    (pizzicato, slap) each hold a channel of their own. A part channel
//    therefore gets a MIDI channel only when it first sounds a note, and its
//    bank, program and controllers are sent along then. When all 15 are taken,
//    the one silent longest is handed over. Drum sets always play on channel 10.
//---------------------------------------------------------

struct OutputChannels {
      struct State {
            int controller[128];
            int program = -1;
            int channel = -1;
            const Channel* articulation = nullptr;
            State() { std::fill(std::begin(controller), std::end(controller), -1); }
            };
      const MasterScore* score = nullptr;
      std::vector<State> parts;
      int owner[16];
      int sounding[16];
      unsigned lastUse[16];
      unsigned clock = 0;

      void reset(const MasterScore* s)
            {
            score = s;
            parts.clear();
            std::fill(std::begin(owner), std::end(owner), -1);
            std::fill(std::begin(sounding), std::end(sounding), 0);
            std::fill(std::begin(lastUse), std::end(lastUse), 0);
            }
      // the part channel behind an index changes when the midi mapping is rebuilt
      // (an instrument change edited, a score reopened): drop what was kept for the old one
      void release(int idx)
            {
            State& st = part(idx);
            if (st.channel >= 0 && st.channel != 9 && owner[st.channel] == idx)
                  owner[st.channel] = -1;
            st = State();
            }
      State& part(int idx)
            {
            if (idx >= int(parts.size()))
                  parts.resize(idx + 1);
            return parts[idx];
            }
      int assign(int idx, bool drums)
            {
            int chan = 9;
            if (!drums) {
                  chan = -1;
                  for (int pass = 0; pass < 3 && chan < 0; ++pass) {
                        for (int c = 0; c < 16; ++c) {
                              if (c == 9)
                                    continue;
                              bool usable = pass == 0 ? owner[c] == -1 : (pass == 1 ? sounding[c] == 0 : true);
                              if (usable && (chan < 0 || lastUse[c] < lastUse[chan]))
                                    chan = c;
                              }
                        }
                  }
            // drum parts share channel 10 instead of taking it from each other
            if (!drums && owner[chan] >= 0 && owner[chan] < int(parts.size()))
                  parts[owner[chan]].channel = -1;
            owner[chan] = idx;
            sounding[chan] = 0;
            part(idx).channel = chan;
            return chan;
            }
      };

static OutputChannels outputChannels;

// TODO: this was copied from Jack version...I'd like to eventually unify these two, so that they handle midi event types in the same manner
void Portaudio::putEvent(const NPlayEvent& e, unsigned framePos)
      {
      PortMidiDriver* portMidiDriver = static_cast<PortMidiDriver*>(midiDriver);
      if (!portMidiDriver || !portMidiDriver->getOutputStream() || !portMidiDriver->canOutput())
            return;

      MasterScore* score = seq->score();
      const int idx = e.channel();
      int portIdx = score->midiPort(idx);

      if (portIdx < 0 ) {
            qDebug("Portaudio::putEvent: invalid port %d", portIdx);
            return;
            }

      auto write = [&](int status, int a, int b) {
            PmError error = Pm_WriteShort(portMidiDriver->getOutputStream(), seq->getCurrentMillisecondTimestampWithLatency(framePos),
                                          Pm_Message(status, less128(a), less128(b)));
            if (error != pmNoError)
                  qDebug("Portaudio: error %d", error);
            };
      // A drum set is bank 128 in the internal synth, bank MSB 1 on the wire;
      // an XG module takes a drum kit from MSB 127 and would play MSB 1 as a melodic voice
      auto controller = [&](int chan, int ctrl, int value) {
            if (ctrl == CTRL_PROGRAM)
                  write(ME_PROGRAM | chan, value, 0);
            else
                  write(ME_CONTROLLER | chan, ctrl, ctrl == CTRL_HBANK && value == 1 ? 127 : value);
            };

      OutputChannels& oc = outputChannels;
      if (oc.score != score)
            oc.reset(score);
      const Channel* articulation = score->midiMapping()[idx].articulation();
      if (oc.part(idx).articulation != articulation) {
            oc.release(idx);
            oc.part(idx).articulation = articulation;
            }
      OutputChannels::State& state = oc.part(idx);
      if (e.type() == ME_CONTROLLER) {
            if (e.dataA() == CTRL_PROGRAM)
                  state.program = e.dataB();
            else if (e.dataA() >= 0 && e.dataA() < 128)
                  state.controller[e.dataA()] = e.dataB();
            }

      const bool noteOn = e.type() == ME_NOTEON && e.velo() > 0;
      const bool drums = score->midiChannel(idx) == 9 || (articulation && articulation->bank() == 128);
      int chan = state.channel;
      // a melodic part must never sound on the drum channel, nor a drum set elsewhere
      if (chan >= 0 && noteOn && (chan == 9) != drums) {
            if (chan != 9 && oc.owner[chan] == idx)
                  oc.owner[chan] = -1;
            state.channel = chan = -1;
            }
      if (chan < 0) {
            if (!noteOn)
                  return;
            chan = oc.assign(idx, drums);
            if (!drums) {
                  // XG part mode Normal: an XG module keeps a part in drum mode once it got bank MSB 127
                  // (a drum set that once played there) and a later bank 0 does not bring it back
                  unsigned char partMode[] = { 0xf0, 0x43, 0x10, 0x4c, 0x08, static_cast<unsigned char>(chan), 0x07, 0x00, 0xf7 };
                  PmError error = Pm_WriteSysEx(portMidiDriver->getOutputStream(),
                                                seq->getCurrentMillisecondTimestampWithLatency(framePos), partMode);
                  if (error != pmNoError)
                        qDebug("Portaudio: error %d", error);
                  }
            if (midiOutputTrace)
                  qDebug("MidiOut assign: index %d (%s) -> channel %d, bank %d/%d, program %d", idx,
                         qPrintable(articulation ? articulation->name() : QString()), chan + 1,
                         state.controller[CTRL_HBANK], state.controller[CTRL_LBANK], state.program);
            for (int ctrl : { int(CTRL_HBANK), int(CTRL_LBANK) })
                  if (state.controller[ctrl] >= 0)
                        controller(chan, ctrl, state.controller[ctrl]);
            if (state.program >= 0)
                  controller(chan, CTRL_PROGRAM, state.program);
            for (int ctrl = 0; ctrl < 128; ++ctrl)
                  if (state.controller[ctrl] >= 0 && ctrl != CTRL_HBANK && ctrl != CTRL_LBANK)
                        controller(chan, ctrl, state.controller[ctrl]);
            }
      if (e.type() == ME_NOTEON || e.type() == ME_NOTEOFF) {
            oc.lastUse[chan] = ++oc.clock;
            if (noteOn)
                  ++oc.sounding[chan];
            else if (oc.sounding[chan] > 0)
                  --oc.sounding[chan];
            }

      if (midiOutputTrace)
            qDebug("MidiOut<%d>: Portaudio: %02x %02x %02x, index %d, chan: %i", portIdx, e.type(), e.dataA(), e.dataB(), idx, chan + 1);

      switch(e.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
            case ME_POLYAFTER:
            case ME_PITCHBEND:
                  write(e.type() | chan, e.dataA(), e.dataB());
                  break;
            case ME_CONTROLLER:
                  if (e.dataA() == CTRL_ALL_NOTES_OFF)
                        oc.sounding[chan] = 0;
                  controller(chan, e.dataA(), e.dataB());
                  break;
            case ME_PROGRAM:
            case ME_AFTERTOUCH:
                  write(e.type() | chan, e.dataA(), 0);
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

