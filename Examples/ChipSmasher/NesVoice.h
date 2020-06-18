//
//  NesVoice.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/20/20.
//

#pragma once

#include "MidiSynth.h"
#include "NesChannel.h"
#include "NesApu.h"

using namespace iplug;

enum EModulations
{
  kModGainSmoother = 0,
  kModSustainSmoother,
  kModLFO,
  kNumModulations,
};

template<typename T>
class NesVoice : public SynthVoice   {
public:
  NesVoice(shared_ptr<Simple_Apu> nesApu, shared_ptr<NesChannels> nesChannels) :
  mAMPEnv("gain", [&](){ mOSC.Reset(); }), // capture ok on RT thread?
  mNesChannels(nesChannels),
  mNesApu(nesApu)
  {
    DBGMSG("new Voice: %i control inputs.\n", static_cast<int>(mInputs.size()));
    // The oscillators are indexed as follows:
    // (0) Square 1, (1) Square 2, (2) Triangle, (3) Noise, (4) DMC.
  }

  void SetChannelEnabled(NesApu::Channel channel, bool enabled) {
    mNesApu->enable_channel(channel, enabled);
  }

  bool GetBusy() const override
  {
    return true;
    // TODO: look into idling for NES voice. Ensure that MIDI activity turns on envelopes
    // return mNesChannels->pulse1.mEnvs.arp.GetState() != NesEnvelope::ENV_OFF; // mAMPEnv.GetBusy();
  }

  void Trigger(double level, bool isRetrigger) override
  {
    DBGMSG("Trigger mKey %d - level %0.2f\n", mKey, level);

    for (auto channel : mNesChannels->allChannels) {
      channel->Trigger(mKey, level);
    }
  }

  void Release() override
  {
    for (auto channel : mNesChannels->allChannels) {
      channel->Release();
    }
  }

  void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
    // inputs to the synthesizer can just fetch a value every block, like this:
    double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
    for (auto channel : mNesChannels->allChannels) {
      channel->SetPitchBend(pitchBend);
    }

    while (mNesApu->samples_avail() < nFrames) {
      for (auto channel : mNesChannels->allChannels) {
        channel->UpdateAPU();
      }
      mNesApu->end_frame();
    }

    mNesApu->read_samples(nesBuffer, nFrames);
    for (int i = 0; i < nFrames; i++) {
      int idx = i + startIdx;
      T smpl = nesBuffer[i] / 32767.0;
      outputs[0][idx] += smpl;
      outputs[1][idx] += smpl;
    }
  }

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
  {
    mOSC.SetSampleRate(sampleRate);
    mAMPEnv.SetSampleRate(sampleRate);
  }

  void SetProgramNumber(int pgm) override
  {
    //TODO:
  }

  // this is called by the VoiceAllocator to set generic control values.
  void SetControl(int controlNumber, float value) override
  {
    //TODO:
  }

public:
  FastSinOscillator<T> mOSC;
  ADSREnvelope<T> mAMPEnv;
  shared_ptr<NesChannels> mNesChannels;
  shared_ptr<Simple_Apu> mNesApu;
  int16_t nesBuffer[32768];

};

