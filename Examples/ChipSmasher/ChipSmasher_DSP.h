#pragma once

#include "MidiSynth.h"
#include "Oscillator.h"
#include "ADSREnvelope.h"
#include "Smoothers.h"
#include "LFO.h"

#include "NesApu.h"
#include "NesVoice.h"
#include "NesDpcm.h"

using namespace iplug;

template<typename T>
class ChipSmasherDSP
{
public:
#pragma mark -
  ChipSmasherDSP(int nVoices)
  {

//    mNesEnvValues = make_shared<array<int, 64>>();
//    mNesEnvelope1 = make_shared<NesEnvelope>(mNesEnvValues);

//    for (auto i = 0; i < nVoices; i++)
//    {
      shared_ptr<Simple_Apu> nesApu = mNesApu = make_shared<Simple_Apu>();
      shared_ptr<NesDpcm> nesDpcm = make_shared<NesDpcm>();

      NesApu::InitializeNoteTables(); // TODO: kill this singleton stuff
      NesApu::InitAndReset(nesApu, 44100, 0, 0, nullptr);
      nesApu->dmc_reader([](void* nesDpcm_, cpu_addr_t addr) -> int {
        return static_cast<NesDpcm*>(nesDpcm_)->GetSampleForAddress(addr - 0xc000);
      }, nesDpcm.get());

      mNesChannels = make_shared<NesChannels>(
        NesChannelPulse(nesApu,    NesApu::Channel::Pulse1,   NesEnvelopes()),
        NesChannelPulse(nesApu,    NesApu::Channel::Pulse2,   NesEnvelopes()),
        NesChannelTriangle(nesApu, NesApu::Channel::Triangle, NesEnvelopes()),
        NesChannelNoise(nesApu,    NesApu::Channel::Noise,    NesEnvelopes()),
        NesChannelDpcm(nesApu,     NesApu::Channel::Dpcm,     nesDpcm)
      );
      SetActiveChannel(NesApu::Channel::Pulse1);

      // Omni Mode
      // add a voice to Zone 0.
      NesVoice<T>* voice = new NesVoice<T>(nesApu, mNesChannels->allChannels);
      mSynth.AddVoice(voice, 0);

      for (int i = 0; i < mNesChannels->allChannels.size(); i++) {
        auto channelVoice = new NesVoice<T>(nesApu, mNesChannels->allChannels[i]);
        auto channelSynth = new MidiSynth(VoiceAllocator::kPolyModeMono, MidiSynth::kDefaultBlockSize);
        mChannelSynths.emplace_back(channelSynth);
        mChannelSynths[i]->AddVoice(channelVoice, 0);
//        mNesChannels->allChannels[i];
      }

    // some MidiSynth API examples:
    // mSynth.SetKeyToPitchFn([](int k){return (k - 69.)/24.;}); // quarter-tone scale
    // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void SetActiveChannel(NesApu::Channel channel) {
//    mSynth.ForEachVoice([=](SynthVoice& v) {
//      auto voice = dynamic_cast<NesVoice<T>&>(v);
      NesChannel* ch = &mNesChannels->pulse1;
      switch (channel) {
        default:
        case NesApu::Channel::Pulse1:
          ch = &mNesChannels->pulse1;
          break;
        case NesApu::Channel::Pulse2:
          ch = &mNesChannels->pulse2;
          break;
        case NesApu::Channel::Triangle:
          ch = &mNesChannels->triangle;
          break;
        case NesApu::Channel::Noise:
          ch = &mNesChannels->noise;
          break;
        case NesApu::Channel::Dpcm:
          ch = &mNesChannels->dpcm;
          break;
      }
      mNesEnvelope1 = &(ch->mEnvs.volume);
      mNesEnvelope2 = &(ch->mEnvs.duty);
      mNesEnvelope3 = &(ch->mEnvs.arp);
      mNesEnvelope4 = &(ch->mEnvs.pitch);
//    });
  }


  void SetChannelEnabled(NesApu::Channel channel, bool enabled) {
    mNesApu->enable_channel(channel, enabled);
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(T));
    }
    
    mParamSmoother.ProcessBlock(mParamsToSmooth, mModulations.GetList(), nFrames);
//    mLFO.ProcessBlock(mModulations.GetList()[kModLFO], nFrames, qnPos, transportIsRunning, tempo);


    if (mOmniMode) {
      mSynth.ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
    } else {
      for (auto &synth : mChannelSynths) {
        synth->ProcessBlock(mModulations.GetList(), outputs, 0, nOutputs, nFrames);
      }
    }

    // NO IDDEA WHAT IM DOING
    while (mNesApu->samples_avail() < nFrames) {
      for (auto channel : mNesChannels->allChannels) {
        channel->UpdateAPU();
      }
      mNesApu->end_frame();
    }

    mNesApu->read_samples(mNesBuffer, nFrames);
    for (int i = 0; i < nFrames; i++) {
      int idx = i;
      T smpl = mNesBuffer[i] / 32767.0;
      outputs[0][idx] += smpl;
      outputs[1][idx] += smpl;
    }

    for(int s=0; s < nFrames;s++)
    {
      T smoothedGain = mModulations.GetList()[kModGainSmoother][s];
      outputs[0][s] *= smoothedGain;
      outputs[1][s] *= smoothedGain;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    if (mOmniMode) {
      mSynth.SetSampleRateAndBlockSize(sampleRate, blockSize);
      mSynth.Reset();
    } else {
      for (auto &synth : mChannelSynths) {
        synth->SetSampleRateAndBlockSize(sampleRate, blockSize);
        synth->Reset();
      }
    }

//    mLFO.SetSampleRate(sampleRate);
    mModulationsData.Resize(blockSize * kNumModulations);
    mModulations.Empty();
    
    for(int i = 0; i < kNumModulations; i++)
    {
      mModulations.Add(mModulationsData.Get() + (blockSize * i));
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    // MIDI event is queued; Corresponds to InstrumentPlayer.PlayNote() in Famistudio
    if (mOmniMode) {
      mSynth.AddMidiMsgToQueue(msg);
    } else {
      mChannelSynths[msg.Channel() % mChannelSynths.size()]->AddMidiMsgToQueue(msg);
    }
  }

  void SetParam(int paramIdx, double value)
  {
//    using EEnvStage = ADSREnvelope<sample>::EStage;

//    if (paramIdx >= kParamEnv1 && paramIdx < kParamEnv2) {
//      int step = paramIdx - kParamEnv1;
//      mNesEnvelope1->mValues[step] = (int)value;
//      return;
//    }
//
//    if (paramIdx >= kParamEnv2 && paramIdx < kParamEnv3) {
//      int step = paramIdx - kParamEnv2;
//      mNesEnvelope2->mValues[step] = (int)value;
//      return;
//    }
//
//    if (paramIdx >= kParamEnv3 && paramIdx < kParamEnv4) {
//      int step = paramIdx - kParamEnv3;
//      mNesEnvelope3->mValues[step] = (int)value;
//      return;
//    }
//
//    if (paramIdx >= kParamEnv4 && paramIdx < kNumParams) {
//      int step = paramIdx - kParamEnv4;
//      mNesEnvelope4->mValues[step] = (int)value;
//      return;
//    }

    switch (paramIdx) {
      case kParamNoteGlideTime:
        mSynth.SetNoteGlideTime(value / 1000.);
        break;
      case kParamGain:
        mParamsToSmooth[kModGainSmoother] = (T) value / 100.;
        break;

      case kParamOmniMode:
        mOmniMode = value > 0.5;
        mSynth.Reset();
        for (auto synth : mChannelSynths) synth->Reset();
        break;

      case kParamPulse1Enabled:
        SetChannelEnabled(NesApu::Channel::Pulse1, value > 0.5);
        break;
      case kParamPulse2Enabled:
        SetChannelEnabled(NesApu::Channel::Pulse2, value > 0.5);
        break;
      case kParamTriangleEnabled:
        SetChannelEnabled(NesApu::Channel::Triangle, value > 0.5);
        break;
      case kParamNoiseEnabled:
        SetChannelEnabled(NesApu::Channel::Noise, value > 0.5);
        break;
      case kParamDpcmEnabled:
        SetChannelEnabled(NesApu::Channel::Dpcm, value > 0.5);
        break;

      case kParamEnv1LoopPoint:
        mNesEnvelope1->SetLoop(value);
        break;
      case kParamEnv1RelPoint:
        mNesEnvelope1->SetRelease(value);
        break;
      case kParamEnv1Length:
        mNesEnvelope1->SetLength(value);
        break;
      case kParamEnv1SpeedDiv:
        mNesEnvelope1->SetSpeedDivider(value);
        break;

      case kParamEnv2LoopPoint:
        mNesEnvelope2->SetLoop(value);
        break;
      case kParamEnv2RelPoint:
        mNesEnvelope2->SetRelease(value);
        break;
      case kParamEnv2Length:
        mNesEnvelope2->SetLength(value);
        break;
      case kParamEnv2SpeedDiv:
        mNesEnvelope2->SetSpeedDivider(value);
        break;

      case kParamEnv3LoopPoint:
        mNesEnvelope3->SetLoop(value);
        break;
      case kParamEnv3RelPoint:
        mNesEnvelope3->SetRelease(value);
        break;
      case kParamEnv3Length:
        mNesEnvelope3->SetLength(value);
        break;
      case kParamEnv3SpeedDiv:
        mNesEnvelope3->SetSpeedDivider(value);
        break;

      case kParamEnv4LoopPoint:
        mNesEnvelope4->SetLoop(value);
        break;
      case kParamEnv4RelPoint:
        mNesEnvelope4->SetRelease(value);
        break;
      case kParamEnv4Length:
        mNesEnvelope4->SetLength(value);
        break;
      case kParamEnv4SpeedDiv:
        mNesEnvelope4->SetSpeedDivider(value);
        break;

      default:
        break;
    }
  }
  
public:
  MidiSynth mSynth { VoiceAllocator::kPolyModeMono, MidiSynth::kDefaultBlockSize };
  WDL_TypedBuf<T> mModulationsData; // Sample data for global modulations (e.g. smoothed sustain)
  WDL_PtrList<T> mModulations; // Ptrlist for global modulations
  LogParamSmooth<T, kNumModulations> mParamSmoother;
  sample mParamsToSmooth[kNumModulations];
//  LFO<T> mLFO;

  NesEnvelope* mNesEnvelope1;
  NesEnvelope* mNesEnvelope2;
  NesEnvelope* mNesEnvelope3;
  NesEnvelope* mNesEnvelope4;

  shared_ptr<NesChannels> mNesChannels;
  vector<MidiSynth*> mChannelSynths;
  shared_ptr<Simple_Apu> mNesApu;
  int16_t mNesBuffer[32768];
  bool mOmniMode;
//  shared_ptr<NesChannel> mActiveNesChannel;
};
