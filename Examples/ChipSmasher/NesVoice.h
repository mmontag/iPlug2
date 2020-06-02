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
  NesVoice(shared_ptr<Simple_Apu> nesApu, shared_ptr<NesEnvelope> nesEnvelope) :
  mAMPEnv("gain", [&](){ mOSC.Reset(); }), // capture ok on RT thread?
  mNesPulse1(nesApu, NesApu::Channel::Pulse1, nesEnvelope),
  mNesPulse2(nesApu, NesApu::Channel::Pulse2, nesEnvelope),
  mNesTriangle(nesApu, NesApu::Channel::Triangle, nesEnvelope),
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
    return mNesPulse1.mNesEnvelope->GetState() != NesEnvelope::ENV_OFF; // mAMPEnv.GetBusy();
  }

  void Trigger(double level, bool isRetrigger) override
  {
//    mOSC.Reset();
    DBGMSG("Trigger mKey %d - level %0.2f\n", mKey, level);
    mNesPulse1.Trigger(mKey);
//    mNesTriangle.Trigger(mKey);
//    Note note {};
//    note.value = mKey - 24;
//    note.isMusical = true;
//    mNesSquare1.PlayNote(note);
//    mNesSquare1.UpdateAPU();

//    mNesTriangle.PlayNote(note);
//    mNesTriangle.UpdateAPU();
//
//    note.value += 7;
//    mNesSquare2.PlayNote(note);
//    mNesSquare2.UpdateAPU();

//    if(isRetrigger)
//      mAMPEnv.Retrigger(level);
//    else
//      mAMPEnv.Start(level);
  }

  void Release() override
  {
//    mAMPEnv.Release();

    mNesPulse1.Release();
//    mNesTriangle.Release();
//    mNesSquare1.note.isMusical = false;
//    mNesSquare1.UpdateAPU();
//
//    mNesSquare2.note.isMusical = false;
//    mNesSquare2.UpdateAPU();
//
//    mNesTriangle.note.isMusical = false;
//    mNesTriangle.UpdateAPU();
  }

  void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
//    // inputs to the synthesizer can just fetch a value every block, like this:
//    //      double gate = mInputs[kVoiceControlGate].endValue;
//    double pitch = mInputs[kVoiceControlPitch].endValue;
//    double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
//    // convert from "1v/oct" pitch space to frequency in Hertz
//    double freq = 440. * pow(2., pitch + pitchBend);
//
//    // make sound output for each output channel
//    for(auto i = startIdx; i < startIdx + nFrames; i++)
//    {
//      // an MPE synth can use pressure here in addition to gain
//      outputs[0][i] += mOSC.Process(osc1Freq) * mAMPEnv.Process(inputs[kModSustainSmoother][i]) * mGain;
//      outputs[1][i] = outputs[0][i];
//    }

    while (mNesApu->samples_avail() < nFrames) {
//      DBGMSG("NES APU %d: %d samples available\n", 0, mNesApu->samples_avail());

      mNesPulse1.UpdateAPU();
//      mNesTriangle.UpdateAPU();
      mNesApu->end_frame();
      // End frame each channel?

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
  NesChannelPulse mNesPulse1;
  NesChannelPulse mNesPulse2;
  NesChannelTriangle mNesTriangle;
//  StepSequencer mStepSeq;
  shared_ptr<Simple_Apu> mNesApu;
  int16_t nesBuffer[32768];

};

