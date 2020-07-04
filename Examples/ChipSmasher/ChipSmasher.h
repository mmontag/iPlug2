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
  kParamOmniMode,

  kParamPulse1Enabled,
  kParamPulse2Enabled,
  kParamTriangleEnabled,
  kParamNoiseEnabled,
  kParamDpcmEnabled,
  kParamVrc6Pulse1Enabled,
  kParamVrc6Pulse2Enabled,
  kParamVrc6SawEnabled,

  kParamPulse1KeyTrack,
  kParamPulse2KeyTrack,
  kParamTriangleKeyTrack,
  kParamNoiseKeyTrack,
  kParamDpcmKeyTrack,
  kParamVrc6Pulse1KeyTrack,
  kParamVrc6Pulse2KeyTrack,
  kParamVrc6SawKeyTrack,

  kParamPulse1VelSens,
  kParamPulse2VelSens,
  kParamTriangleVelSens,
  kParamNoiseVelSens,
  kParamDpcmVelSens,
  kParamVrc6Pulse1VelSens,
  kParamVrc6Pulse2VelSens,
  kParamVrc6SawVelSens,

  kParamPulse1Legato,
  kParamPulse2Legato,
  kParamTriangleLegato,
  kParamNoiseLegato,
  kParamDpcmLegato,
  kParamVrc6Pulse1Legato,
  kParamVrc6Pulse2Legato,
  kParamVrc6SawLegato,

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
  kNumParams
};

#if IPLUG_DSP
// will use EParams in ChipSmasher_DSP.h
#include "ChipSmasher_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagKeyboard = 0,
  kCtrlTagBender,
  kCtrlTagModWheel,
  kCtrlTagKeyTrack,
  kCtrlTagVelSens,
  kCtrlTagLegato,
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

  void OnPresetsModified() override;

  int UnserializeState(const IByteChunk &chunk, int startPos) override;

private:
  ChipSmasherDSP<sample> mDSP;
  // TODO: Figure out why ISender works best with queue size 8
  ISender<1, 8, int> mEnvelopeVisSender;
#endif

  void UpdateStepSequencers();
};
