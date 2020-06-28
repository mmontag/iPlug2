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
  kNumModulations,
};

template<typename T>
class NesVoice : public SynthVoice   {
public:
  NesVoice(shared_ptr<Simple_Apu> nesApu, vector<NesChannel*> &nesChannels) :
  mAMPEnv("gain", [&](){ mOSC.Reset(); }), // capture ok on RT thread?
  mNesChannels(nesChannels),
  mNesApu(nesApu)
  {
    DBGMSG("new Voice: %i control inputs.\n", static_cast<int>(mInputs.size()));
    // The oscillators are indexed as follows:
    // (0) Square 1, (1) Square 2, (2) Triangle, (3) Noise, (4) DMC.
  }

  bool GetBusy() const override
  {
    return true;
    // TODO: look into idling for NES voice. Ensure that MIDI activity turns on envelopes
    // return mNesChannels->pulse1.mEnvs.arp.GetState() != NesEnvelope::ENV_OFF;
  }

  void Trigger(double level, bool isRetrigger) override
  {
    DBGMSG("Trigger mKey %d - level %0.2f\n", mKey, level);

    for (auto channel : mNesChannels) {
      channel->Trigger(mKey, level);
    }
  }

  void Release() override
  {
    for (auto channel : mNesChannels) {
      channel->Release();
    }
  }

  void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
    // inputs to the synthesizer can just fetch a value every block, like this:
    double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
    for (auto channel : mNesChannels) {
      channel->SetPitchBend(pitchBend);
    }
  }

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
  {
    mOSC.SetSampleRate(sampleRate);
    mAMPEnv.SetSampleRate(sampleRate);
  }

public:
  FastSinOscillator<T> mOSC;
  ADSREnvelope<T> mAMPEnv;
  vector<NesChannel*> mNesChannels;
  shared_ptr<Simple_Apu> mNesApu;

};

