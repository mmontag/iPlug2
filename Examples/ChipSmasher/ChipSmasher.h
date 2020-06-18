#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

const int kNumPrograms = 8;

const int kEnvelopeSteps = 64;
const int kNumEnvParams = 68;

enum EParams
{
  kParamGain = 0,
  kParamNoteGlideTime,
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kParamLFOShape,
  kParamLFORateHz,
  kParamLFORateTempo,
  kParamLFORateMode,
  kParamLFODepth,
  kParamPulse1Enabled,
  kParamPulse2Enabled,
  kParamTriangleEnabled,
  kParamNoiseEnabled,
  kParamDpcmEnabled,


  kParamEnv1LoopPoint,
  kParamEnv1RelPoint,
  kParamEnv1Length,
  kParamEnv1SpeedDiv,

  kParamEnv2LoopPoint,
  kParamEnv2RelPoint,
  kParamEnv2Length,
  kParamEnv2SpeedDiv,

  kParamEnv3LoopPoint,
  kParamEnv3RelPoint,
  kParamEnv3Length,
  kParamEnv3SpeedDiv,

  kParamEnv4LoopPoint,
  kParamEnv4RelPoint,
  kParamEnv4Length,
  kParamEnv4SpeedDiv,

  kParamEnv1,
  kParamEnv2 = kParamEnv1 + kEnvelopeSteps,
  kParamEnv3 = kParamEnv2 + kEnvelopeSteps,
  kParamEnv4 = kParamEnv3 + kEnvelopeSteps,
  kNumParams = kParamEnv4 + kEnvelopeSteps
};

#if IPLUG_DSP
// will use EParams in ChipSmasher_DSP.h
#include "ChipSmasher_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagLFOVis,
  kCtrlTagScope,
  kCtrlTagRTText,
  kCtrlTagKeyboard,
  kCtrlTagBender,
  kCtrlTagEnvelope1,
  kCtrlTagEnvelope2,
  kCtrlTagEnvelope3,
  kCtrlTagEnvelope4,
  kCtrlTagDpcmEditor,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class ChipSmasher final : public Plugin
{
public:
  ChipSmasher(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  bool SerializeState(IByteChunk &chunk) const override;

private:
  ChipSmasherDSP<sample> mDSP {1};
  IPeakSender<2> mMeterSender;
  ISender<1> mLFOVisSender;
  ISender<1, 8, int> mEnvelopeVisSender;
#endif
};
