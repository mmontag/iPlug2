#include "ChipSmasher.h"
#include "IPlug_include_in_plug_src.h"
#include "StepSequencer.h"
#include "DpcmEditorControl.h"
#include "KnobControl.h"

ChipSmasher::ChipSmasher(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)->InitMilliseconds("Note Glide Time", 0., 0.0, 30.);

  GetParam(kParamPulse1Enabled)->InitBool("Pulse 1 Enabled", false);
  GetParam(kParamPulse2Enabled)->InitBool("Pulse 2 Enabled", false);
  GetParam(kParamTriangleEnabled)->InitBool("Triangle Enabled", false);
  GetParam(kParamNoiseEnabled)->InitBool("Noise Enabled", false);
  GetParam(kParamDpcmEnabled)->InitBool("DPCM Enabled", false);
  GetParam(kParamVrc6Pulse1Enabled)->InitBool("VRC6 Pulse 1 Enabled", false);
  GetParam(kParamVrc6Pulse2Enabled)->InitBool("VRC6 Pulse 2 Enabled", false);
  GetParam(kParamVrc6SawEnabled)->InitBool("VRC6 Saw Enabled", false);

  GetParam(kParamPulse1KeyTrack)->InitBool("Pulse 1 Key Track", true);
  GetParam(kParamPulse2KeyTrack)->InitBool("Pulse 2 Key Track", true);
  GetParam(kParamTriangleKeyTrack)->InitBool("Triangle Key Track", true);
  GetParam(kParamNoiseKeyTrack)->InitBool("Noise Key Track", true);
  GetParam(kParamDpcmKeyTrack)->InitBool("DPCM Key Track", true);
  GetParam(kParamVrc6Pulse1KeyTrack)->InitBool("VRC6 Pulse 1 Key Track", true);
  GetParam(kParamVrc6Pulse2KeyTrack)->InitBool("VRC6 Pulse 2 Key Track", true);
  GetParam(kParamVrc6SawKeyTrack)->InitBool("VRC6 Saw Key Track", true);

  GetParam(kParamPulse1VelSens)->InitBool("Pulse 1 Vel Sens", true);
  GetParam(kParamPulse2VelSens)->InitBool("Pulse 2 Vel Sens", true);
  GetParam(kParamTriangleVelSens)->InitBool("Triangle Vel Sens", true);
  GetParam(kParamNoiseVelSens)->InitBool("Noise Vel Sens", true);
  GetParam(kParamDpcmVelSens)->InitBool("DPCM Vel Sens", true);
  GetParam(kParamVrc6Pulse1VelSens)->InitBool("VRC6 Pulse 1 Vel Sens", true);
  GetParam(kParamVrc6Pulse2VelSens)->InitBool("VRC6 Pulse 2 Vel Sens", true);
  GetParam(kParamVrc6SawVelSens)->InitBool("VRC6 Saw Vel Sens", true);

  GetParam(kParamPulse1Legato)->InitBool("Pulse 1 Legato", false);
  GetParam(kParamPulse2Legato)->InitBool("Pulse 2 Legato", false);
  GetParam(kParamTriangleLegato)->InitBool("Triangle Legato", false);
  GetParam(kParamNoiseLegato)->InitBool("Noise Legato", false);
  GetParam(kParamDpcmLegato)->InitBool("DPCM Legato", false);
  GetParam(kParamVrc6Pulse1Legato)->InitBool("VRC6 Pulse 1 Legato", false);
  GetParam(kParamVrc6Pulse2Legato)->InitBool("VRC6 Pulse 2 Legato", false);
  GetParam(kParamVrc6SawLegato)->InitBool("VRC6 Saw Legato", false);

  GetParam(kParamOmniMode)->InitBool("Omni Mode Enabled", true);

  GetParam(kParamEnv1LoopPoint)->InitInt("Env 1 Loop",    15, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv1RelPoint)->InitInt( "Env 1 Release", 16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv1Length)->InitInt(   "Env 1 Length",  16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv1SpeedDiv)->InitInt( "Env 1 Speed",   1,  1, 8,  "", IParam::kFlagStepped);

  GetParam(kParamEnv2LoopPoint)->InitInt("Env 2 Loop",    15, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv2RelPoint)->InitInt( "Env 2 Release", 16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv2Length)->InitInt(   "Env 2 Length",  16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv2SpeedDiv)->InitInt( "Env 2 Speed",   1,  1, 8,  "", IParam::kFlagStepped);

  GetParam(kParamEnv3LoopPoint)->InitInt("Env 3 Loop",    15, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv3RelPoint)->InitInt( "Env 3 Release", 16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv3Length)->InitInt(   "Env 3 Length",  16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv3SpeedDiv)->InitInt( "Env 3 Speed",   1,  1, 8,  "", IParam::kFlagStepped);

  GetParam(kParamEnv4LoopPoint)->InitInt("Env 4 Loop",    15, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv4RelPoint)->InitInt( "Env 4 Release", 16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv4Length)->InitInt(   "Env 4 Length",  16, 0, 64, "", IParam::kFlagStepped);
  GetParam(kParamEnv4SpeedDiv)->InitInt( "Env 4 Speed",   1,  1, 8,  "", IParam::kFlagStepped);

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
//    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableMultiTouch(true);

#ifdef OS_WEB
    pGraphics->AttachPopupMenuControl();
#endif

    const IVStyle style {
      true, // Show label
      true, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        COLOR_BLACK, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(11.f, DEFAULT_TEXT_FGCOLOR, "Univers", EAlign::Near, EVAlign::Middle), // Label text
      IText(15.f, DEFAULT_TEXT_FGCOLOR, "Normal", EAlign::Center, EVAlign::Middle),
      false, // Hide mouse
      true,  // Show frame
      false, // Show shadows
      DEFAULT_EMBOSS,
      DEFAULT_ROUNDNESS,
      DEFAULT_FRAME_THICKNESS,
      DEFAULT_SHADOW_OFFSET,
      DEFAULT_WIDGET_FRAC
    };

    const IVStyle noLabelStyle {
      false, // Show label
      false, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        COLOR_BLACK, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(12.f, EAlign::Center), // Label text
      DEFAULT_VALUE_TEXT,
      false, // Hide mouse
      true,  // Show frame
      false  // Show shadows
    };

//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("Univers", UNIVERS_FN);
    pGraphics->LoadFont("Normal", NORMAL_FN);
    pGraphics->LoadFont("Bold", BOLD_FN);
    pGraphics->EnableTooltips(true);

    IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(
      new IPanelControl(b, IPattern::CreateLinearGradient(0, 0, b.W(), b.H(),
                                                          {
                                                            IColorStop(IColor::FromColorCodeStr("#C0C0C0"), 0),
                                                            IColorStop(IColor::FromColorCodeStr("#828282"), 1)
                                                          }))
    );
    b = pGraphics->GetBounds().GetPadded(-PLUG_PADDING);

#pragma mark - Keyboard

    IRECT keyboardBounds = b.GetFromBottom(120);
    IRECT wheelsBounds = keyboardBounds.ReduceFromLeft(100.f);
    pGraphics->AttachControl(new IVKeyboardControl(keyboardBounds, 24, 96, false), kCtrlTagKeyboard);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5)), kCtrlTagBender);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5, true),
                                               IMidiMsg::EControlChangeMsg::kModWheel), kCtrlTagModWheel);
    pGraphics->SetQwertyMidiKeyHandlerFunc([pGraphics](const IMidiMsg &msg) {
      dynamic_cast<IVKeyboardControl *>(
        pGraphics->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(
        msg.NoteNumber(), msg.StatusMsg() == IMidiMsg::kNoteOn);
    });

#pragma mark - Channel Panel

    b = b.GetReducedFromBottom(137);
    IRECT channelPanel = b.GetFromLeft(84);

    IRECT channelButtonRect = channelPanel.GetFromTop(40.f);
    for (auto paramTuples : vector<tuple<int, NesApu::Channel, string>>{{kParamPulse1Enabled,     NesApu::Channel::Pulse1,     "Pulse 1"},
                                                                        {kParamPulse2Enabled,     NesApu::Channel::Pulse2,     "Pulse 2"},
                                                                        {kParamTriangleEnabled,   NesApu::Channel::Triangle,   "Triangle"},
                                                                        {kParamNoiseEnabled,      NesApu::Channel::Noise,      "Noise"},
                                                                        {kParamDpcmEnabled,       NesApu::Channel::Dpcm,       "DPCM"},
                                                                        {kParamVrc6Pulse1Enabled, NesApu::Channel::Vrc6Pulse1, "Pulse 3"},
                                                                        {kParamVrc6Pulse2Enabled, NesApu::Channel::Vrc6Pulse2, "Pulse 4"},
                                                                        {kParamVrc6SawEnabled,    NesApu::Channel::Vrc6Saw,    "Saw"}}) {
      auto param = get<int>(paramTuples);
      auto channel = get<NesApu::Channel>(paramTuples);
      auto label = get<string>(paramTuples).c_str();
      pGraphics->AttachControl(new IVToggleControl(channelButtonRect.GetFromRight(40.f), param, label, noLabelStyle), kNoTag, "NES");
      pGraphics->AttachControl(new IVButtonControl(channelButtonRect.GetReducedFromRight(40.f), [this, channel, param](IControl* pCaller){
        bool isDpcm = channel == NesApu::Channel::Dpcm;
        GetUI()->GetControlWithTag(kCtrlTagDpcmEditor)->Hide(!isDpcm);
        GetUI()->ForControlInGroup("StepSequencers", [=](IControl& control) {
          control.Hide(isDpcm);
        });
        GetUI()->ForControlInGroup("Knobs", [=](IControl& control) {
          control.Hide(isDpcm);
        });

        // Reassign channel-specific toggles
        GetUI()->GetControlWithTag(kCtrlTagKeyTrack)->SetParamIdx(param - kParamPulse1Enabled + kParamPulse1KeyTrack);
        GetUI()->GetControlWithTag(kCtrlTagVelSens)->SetParamIdx(param - kParamPulse1Enabled + kParamPulse1VelSens);
        GetUI()->GetControlWithTag(kCtrlTagLegato)->SetParamIdx(param - kParamPulse1Enabled + kParamPulse1Legato);

        mDSP.SetActiveChannel(channel);
        UpdateStepSequencers();
        SendCurrentParamValuesFromDelegate();
      }, label, style), kNoTag, "NES");
      channelButtonRect.Translate(0, channelButtonRect.H());
    }

    auto keyTrackButton = new IVToggleControl(channelButtonRect, kParamPulse1KeyTrack, "Key Track", style);
    pGraphics->AttachControl(keyTrackButton, kCtrlTagKeyTrack, "NES");
    channelButtonRect.Translate(0, channelButtonRect.H());

    auto velSensButton = new IVToggleControl(channelButtonRect, kParamPulse1VelSens, "Vel Sens", style);
    pGraphics->AttachControl(velSensButton, kCtrlTagVelSens, "NES");
    channelButtonRect.Translate(0, channelButtonRect.H());

    auto legatoButton = new IVToggleControl(channelButtonRect, kParamPulse1Legato, "Legato", style);
    pGraphics->AttachControl(legatoButton, kCtrlTagLegato, "NES");
    channelButtonRect.Translate(0, channelButtonRect.H());

    auto omniButton = new IVToggleControl(channelButtonRect, kParamOmniMode, "Omni Mode", style);
    omniButton->SetTooltip("When Omni Mode is on, all NES channels receive events from all MIDI channels. "
                           "When Omni Mode is off, each MIDI channel is mapped to a different NES channel. \n"
                           "If your plugin host doesn't support multichannel plugins, "
                           "or if you don't know what this means, leave Omni Mode on.");
    pGraphics->AttachControl(omniButton, kNoTag, "NES");
    channelButtonRect.Translate(0, channelButtonRect.H());

    pGraphics->AttachControl(new IVButtonControl(channelButtonRect, [=](IControl *pCaller) {
      static bool hide = false;
      hide = !hide;
      pGraphics->GetControlWithTag(kCtrlTagKeyboard)->Hide(hide);
      pGraphics->GetControlWithTag(kCtrlTagBender)->Hide(hide);
      pGraphics->GetControlWithTag(kCtrlTagModWheel)->Hide(hide);
      pGraphics->Resize(PLUG_WIDTH, hide ? PLUG_HEIGHT - keyboardBounds.H() - PLUG_PADDING : PLUG_HEIGHT, pGraphics->GetDrawScale());
    }, "Toggle Keyboard", DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE)));

#pragma mark - Presets

    MakeDefaultPreset(nullptr, kNumPrograms);

    pGraphics->AttachControl(new IVBakedPresetManagerControl(b.ReduceFromTop(40).GetFromRight(300), style.WithLabelText({15.f, EVAlign::Middle}))); // "./presets", "nesvst"));

#pragma mark - Step Sequencers

    const int kKnobHeight = 64.f;

    const IRECT editorPanel = b.GetReducedFromLeft(100);

    auto createEnvelopePanel = [=](IRECT rect, const char* label, float minVal, float maxVal, NesEnvelope* nesEnv, int lpP, int rpP, int lP, int sdP, int ctrlTag, IColor color) {

      auto stepSeq = new StepSequencer(rect.GetReducedFromBottom(kKnobHeight + 16.f),
                                       label,
                                       style.WithColor(kFG, color).WithColor(kBG, IColor::FromColorCodeStr("#141414")),
                                       64,
                                       1.f/float(maxVal - minVal),
                                       nullptr);
      pGraphics->AttachControl(stepSeq, ctrlTag, "StepSequencers");

      IRECT knobBox = rect.GetFromBottom(kKnobHeight);
      IVStyle knobStyle = style
        .WithLabelText(style.labelText.WithAlign(EAlign::Center).WithFont("Normal").WithSize(15.f))
        .WithValueText(style.valueText.WithFont("Bold"));

      // Loop
      auto lpC = new KnobControl(knobBox.SubRectHorizontal(4, 0), lpP, "Loop", knobStyle, false, false);
      lpC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetLoopPoint(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(lpC, kNoTag, "Knobs");
      stepSeq->SetLoopPoint(GetParam(kParamEnv1LoopPoint)->Int());

      // Release
      auto rpC = new KnobControl(knobBox.SubRectHorizontal(4, 1), rpP, "Release", knobStyle, false, false);
      rpC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetReleasePoint(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(rpC, kNoTag, "Knobs");
      stepSeq->SetReleasePoint(GetParam(kParamEnv1LoopPoint)->Int());

      // Length
      auto lC = new KnobControl(knobBox.SubRectHorizontal(4, 2), lP, "Length", knobStyle, false, false);
      lC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetLength(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(lC, kNoTag, "Knobs");
      stepSeq->SetLength(GetParam(kParamEnv1LoopPoint)->Int());

      // Speed
      auto sC = new KnobControl(knobBox.SubRectHorizontal(4, 3), sdP, "Speed", knobStyle, false, false);
      pGraphics->AttachControl(sC, kNoTag, "Knobs");
    };

    IRECT envPanel = editorPanel.GetPadded(8);
    createEnvelopePanel(envPanel.GetGridCell(0, 2, 2).GetPadded(-8), "VOLUME", 0, 15, mDSP.mNesEnvelope1,
      kParamEnv1LoopPoint, kParamEnv1RelPoint, kParamEnv1Length, kParamEnv1SpeedDiv, kCtrlTagEnvelope1, IColor::FromColorCodeStr("#CC2626"));
    createEnvelopePanel(envPanel.GetGridCell(1, 2, 2).GetPadded(-8), "DUTY", 0, 7, mDSP.mNesEnvelope2,
      kParamEnv2LoopPoint, kParamEnv2RelPoint, kParamEnv2Length, kParamEnv2SpeedDiv, kCtrlTagEnvelope2, IColor::FromColorCodeStr("#DE5E33"));
    createEnvelopePanel(envPanel.GetGridCell(2, 2, 2).GetPadded(-8), "PITCH", -12, 12, mDSP.mNesEnvelope3,
      kParamEnv3LoopPoint, kParamEnv3RelPoint, kParamEnv3Length, kParamEnv3SpeedDiv, kCtrlTagEnvelope3, IColor::FromColorCodeStr("#53AD8E"));
    createEnvelopePanel(envPanel.GetGridCell(3, 2, 2).GetPadded(-8), "FINE PITCH", -12, 12, mDSP.mNesEnvelope4,
      kParamEnv4LoopPoint, kParamEnv4RelPoint, kParamEnv4Length, kParamEnv4SpeedDiv, kCtrlTagEnvelope4, IColor::FromColorCodeStr("#747ACD"));

    UpdateStepSequencers();

#pragma mark - DPCM Editor

    auto dpcmEditor = new DpcmEditorControl(editorPanel, style, mDSP.mNesChannels->dpcm.mNesDpcm);
    pGraphics->AttachControl(dpcmEditor, kCtrlTagDpcmEditor, "DpcmEditor");
    dpcmEditor->Hide(true);

  };
#endif
}

void ChipSmasher::OnPresetsModified() {
  printf("- PRESETS MODIFIED -\n");
  UpdateStepSequencers();
  GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });

  IPluginBase::OnPresetsModified();
}

void ChipSmasher::UpdateStepSequencers() {
  // update all envelope values
  for (auto ctrlToEnv : vector<pair<int, NesEnvelope *>>{{kCtrlTagEnvelope1, mDSP.mNesEnvelope1},
                                                         {kCtrlTagEnvelope2, mDSP.mNesEnvelope2},
                                                         {kCtrlTagEnvelope3, mDSP.mNesEnvelope3},
                                                         {kCtrlTagEnvelope4, mDSP.mNesEnvelope4}}) {
    auto seq = dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(ctrlToEnv.first));
    auto nesEnv = ctrlToEnv.second;
    for (int i = 0; i < 64; i++) {
      seq->SetValue(float(nesEnv->mValues[i] - nesEnv->mMinVal) / float(nesEnv->mMaxVal - nesEnv->mMinVal), i);
    }
    seq->SetLoopPoint(nesEnv->mLoopPoint);
    seq->SetReleasePoint(nesEnv->mReleasePoint);
    seq->SetLength(nesEnv->mLength);
    seq->SetActionFunc([nesEnv](int stepIdx, float value) {
      nesEnv->mValues[stepIdx] = (int)iplug::Lerp((float)nesEnv->mMinVal, (float)nesEnv->mMaxVal, value);
    });
    seq->SetSlidersDirty();
  }

  // update all env params sliders/knobs
  GetParam(kParamEnv1LoopPoint)->Set(mDSP.mNesEnvelope1->mLoopPoint);
  GetParam(kParamEnv2LoopPoint)->Set(mDSP.mNesEnvelope2->mLoopPoint);
  GetParam(kParamEnv3LoopPoint)->Set(mDSP.mNesEnvelope3->mLoopPoint);
  GetParam(kParamEnv4LoopPoint)->Set(mDSP.mNesEnvelope4->mLoopPoint);

  GetParam(kParamEnv1RelPoint)->Set(mDSP.mNesEnvelope1->mReleasePoint);
  GetParam(kParamEnv2RelPoint)->Set(mDSP.mNesEnvelope2->mReleasePoint);
  GetParam(kParamEnv3RelPoint)->Set(mDSP.mNesEnvelope3->mReleasePoint);
  GetParam(kParamEnv4RelPoint)->Set(mDSP.mNesEnvelope4->mReleasePoint);

  GetParam(kParamEnv1Length)->Set(mDSP.mNesEnvelope1->mLength);
  GetParam(kParamEnv2Length)->Set(mDSP.mNesEnvelope2->mLength);
  GetParam(kParamEnv3Length)->Set(mDSP.mNesEnvelope3->mLength);
  GetParam(kParamEnv4Length)->Set(mDSP.mNesEnvelope4->mLength);

  GetParam(kParamEnv1SpeedDiv)->Set(mDSP.mNesEnvelope1->mSpeedDivider);
  GetParam(kParamEnv2SpeedDiv)->Set(mDSP.mNesEnvelope2->mSpeedDivider);
  GetParam(kParamEnv3SpeedDiv)->Set(mDSP.mNesEnvelope3->mSpeedDivider);
  GetParam(kParamEnv4SpeedDiv)->Set(mDSP.mNesEnvelope4->mSpeedDivider);
}

#if IPLUG_DSP
void ChipSmasher::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames, mTimeInfo.mPPQPos, mTimeInfo.mTransportIsRunning);
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope1, {mDSP.mNesEnvelope1->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope2, {mDSP.mNesEnvelope2->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope3, {mDSP.mNesEnvelope3->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope4, {mDSP.mNesEnvelope4->GetStep()}});
  mEnvelopeVisSender.TransmitData(*this);
}

void ChipSmasher::OnIdle()
{
//  mEnvelopeVisSender.TransmitData(*this);
}

void ChipSmasher::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void ChipSmasher::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;

  int status = msg.StatusMsg();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }

handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void ChipSmasher::OnParamChange(int paramIdx)
{
//  printf("OnParamChange paramIdx %d - value %d\n", paramIdx, GetParam(paramIdx)->Int());
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

bool ChipSmasher::SerializeState(IByteChunk &chunk) const {
  for (auto channel : mDSP.mNesChannels->allChannels) {
    channel->Serialize(chunk);
  }
  return IPluginBase::SerializeState(chunk);
}

int ChipSmasher::UnserializeState(const IByteChunk &chunk, int startPos) {
  int pos = startPos;
  for (auto channel : mDSP.mNesChannels->allChannels) {
    pos = channel->Deserialize(chunk, pos);
  }
  return IPluginBase::UnserializeState(chunk, pos);
}

bool ChipSmasher::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  printf("OnMessage msgTag %d - ctrlTag %d - dataSize %d\n", msgTag, ctrlTag, dataSize);
  if(ctrlTag == kCtrlTagBender && msgTag == IWheelControl::kMessageTagSetPitchBendRange)
  {
    const int bendRange = *static_cast<const int*>(pData);
    mDSP.mSynth.SetPitchBendRange(bendRange);
  }

  return false;
}
#endif
