#pragma once

#include "MidiSynth.h"
#include "NesApu.h"
#include "NesVoice.h"
#include "NesDpcm.h"

using namespace iplug;

template<typename T>
class ChipSmasherDSP
{
public:
#pragma mark -
  ChipSmasherDSP()
  {
      shared_ptr<Simple_Apu> nesApu = mNesApu = make_shared<Simple_Apu>();
      shared_ptr<NesDpcm> nesDpcm = make_shared<NesDpcm>();

      NesApu::InitializeNoteTables(); // TODO: kill this singleton stuff
      NesApu::InitAndReset(nesApu, 44100, NesApu::APU_EXPANSION_VRC6, 0, nullptr);
      nesApu->dmc_reader([](void* nesDpcm_, cpu_addr_t addr) -> int {
        return static_cast<NesDpcm*>(nesDpcm_)->GetSampleForAddress(addr - 0xc000);
      }, nesDpcm.get());

      mNesChannels = make_shared<NesChannels>(
        NesChannelPulse(nesApu,     NesApu::Channel::Pulse1,      NesEnvelopes()),
        NesChannelPulse(nesApu,     NesApu::Channel::Pulse2,      NesEnvelopes()),
        NesChannelTriangle(nesApu,  NesApu::Channel::Triangle,    NesEnvelopes()),
        NesChannelNoise(nesApu,     NesApu::Channel::Noise,       NesEnvelopes()),
        NesChannelDpcm(nesApu,      NesApu::Channel::Dpcm,        nesDpcm),
        NesChannelVrc6Pulse(nesApu, NesApu::Channel::Vrc6Pulse1, NesEnvelopes()),
        NesChannelVrc6Pulse(nesApu, NesApu::Channel::Vrc6Pulse2, NesEnvelopes()),
        NesChannelVrc6Saw(nesApu,   NesApu::Channel::Vrc6Saw,     NesEnvelopes())
      );
      SetActiveChannel(NesApu::Channel::Pulse1);

      for (int i = 0; i < mNesChannels->numChannels; i++) {
        auto channelVoice = new NesVoice<T>(nesApu, mNesChannels->allChannels[i]);
        auto channelSynth = new MidiSynth(VoiceAllocator::kPolyModeMono, MidiSynth::kDefaultBlockSize);
        mChannelSynths.emplace_back(channelSynth);
        mChannelSynths[i]->AddVoice(channelVoice, 0);
      }

      // TODO: Paraphonic mode? (one synth with several voices; assign each voice to a different NES channel)

      // TODO: Portamento?
      // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void SetActiveChannel(NesApu::Channel channel) {
    for (auto ch : mNesChannels->allChannels) {
      if (ch->mChannel == channel) {
        mNesEnvelope1 = &(ch->mEnvs.volume);
        mNesEnvelope2 = &(ch->mEnvs.duty);
        mNesEnvelope3 = &(ch->mEnvs.arp);
        mNesEnvelope4 = &(ch->mEnvs.pitch);
        break;
      }
    }
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

    for (auto &synth : mChannelSynths) {
      synth->ProcessBlock(nullptr, outputs, 0, nOutputs, nFrames);
    }

    while (mNesApu->samples_avail() < nFrames) {
      for (auto channel : mNesChannels->allChannels) {
        channel->UpdateAPU();
      }
      // TODO: this updates the APU state at 60 hz, introducing jitter and up to 16ms latency. acceptable?
      mNesApu->end_frame();
    }

    mNesApu->read_samples(mNesBuffer, nFrames);
    for (int i = 0; i < nFrames; i++) {
      int idx = i;
      T smpl = mNesBuffer[i] / 32767.0;
      outputs[0][idx] += smpl;
      outputs[1][idx] += smpl;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    for (auto &synth : mChannelSynths) {
      synth->SetSampleRateAndBlockSize(sampleRate, blockSize);
      synth->Reset();
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    if (mOmniMode) {
      for (auto synth : mChannelSynths) synth->AddMidiMsgToQueue(msg);
    } else if (msg.Channel() < mNesChannels->numChannels) {
      mChannelSynths[msg.Channel()]->AddMidiMsgToQueue(msg);
    }
  }

  void SetParam(int paramIdx, double value)
  {
    switch (paramIdx) {
      case kParamNoteGlideTime:
        for (auto synth : mChannelSynths) synth->SetNoteGlideTime(value / 1000.);
        break;

      case kParamOmniMode:
        mOmniMode = value > 0.5;
        for (auto synth : mChannelSynths) synth->Reset();
        break;

      case kParamPulse1Enabled:
      case kParamPulse2Enabled:
      case kParamTriangleEnabled:
      case kParamNoiseEnabled:
      case kParamDpcmEnabled:
      case kParamVrc6Pulse1Enabled:
      case kParamVrc6Pulse2Enabled:
      case kParamVrc6SawEnabled:
        SetChannelEnabled(NesApu::Channel(paramIdx - kParamPulse1Enabled), value > 0.5);
        break;

      case kParamPulse1KeyTrack:
      case kParamPulse2KeyTrack:
      case kParamTriangleKeyTrack:
      case kParamNoiseKeyTrack:
      case kParamDpcmKeyTrack:
      case kParamVrc6Pulse1KeyTrack:
      case kParamVrc6Pulse2KeyTrack:
      case kParamVrc6SawKeyTrack:
        mNesChannels->allChannels[paramIdx - kParamPulse1KeyTrack]->SetKeyTrack(value > 0.5);
        break;

      case kParamPulse1VelSens:
      case kParamPulse2VelSens:
      case kParamTriangleVelSens:
      case kParamNoiseVelSens:
      case kParamDpcmVelSens:
      case kParamVrc6Pulse1VelSens:
      case kParamVrc6Pulse2VelSens:
      case kParamVrc6SawVelSens:
        mNesChannels->allChannels[paramIdx - kParamPulse1VelSens]->SetVelSens(value > 0.5);
        break;

      case kParamPulse1Legato:
      case kParamPulse2Legato:
      case kParamTriangleLegato:
      case kParamNoiseLegato:
      case kParamDpcmLegato:
      case kParamVrc6Pulse1Legato:
      case kParamVrc6Pulse2Legato:
      case kParamVrc6SawLegato:
        mChannelSynths[paramIdx - kParamPulse1Legato]->SetLegato(value > 0.5);
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
  NesEnvelope* mNesEnvelope1;
  NesEnvelope* mNesEnvelope2;
  NesEnvelope* mNesEnvelope3;
  NesEnvelope* mNesEnvelope4;

  shared_ptr<NesChannels> mNesChannels;
  vector<MidiSynth*> mChannelSynths;
  shared_ptr<Simple_Apu> mNesApu;
  int16_t mNesBuffer[32768];
  bool mOmniMode;
};
