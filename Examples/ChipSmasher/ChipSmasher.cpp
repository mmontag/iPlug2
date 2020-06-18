#include "ChipSmasher.h"
#include "IPlug_include_in_plug_src.h"
#include "StepSequencer.h"
#include "IDpcmEditorControl.h"

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

  // Envelope steps
  for (int i = 0; i < 64; i++) {
    GetParam(kParamEnv1 + i)->InitInt("Volume Env", 15, 0, 15);
  }

  for (int i = 0; i < 64; i++) {
    GetParam(kParamEnv2 + i)->InitInt("Duty Env", 2, 0, 7);
  }

  for (int i = 0; i < 64; i++) {
    GetParam(kParamEnv3 + i)->InitInt("Arp Env", 0, -12, 12);
  }

  for (int i = 0; i < 64; i++) {
    GetParam(kParamEnv4 + i)->InitInt("Fine Pitch Env", 0, -12, 12);
  }

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
      IText(11.f, DEFAULT_TEXT_FGCOLOR, "Univers", EAlign::Near, EVAlign::Top), // Label text
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
//    pGraphics->LoadFont("Inter-Medium", INTER_MEDIUM_FN);
//    pGraphics->LoadFont("Inter-Bold", INTER_BOLD_FN);
    pGraphics->LoadFont("Normal", NORMAL_FN);
    pGraphics->LoadFont("Bold", BOLD_FN);

    IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(
      new IPanelControl(b, IPattern::CreateLinearGradient(0, 0, b.W(), b.H(),
                                                          {
                                                            IColorStop(IColor::FromColorCodeStr("#C0C0C0"), 0),
                                                            IColorStop(IColor::FromColorCodeStr("#828282"), 1)
                                                          }))
    );

    b = pGraphics->GetBounds().GetPadded(-20.f);
    IRECT keyboardBounds = b.GetFromBottom(120);
    IRECT wheelsBounds = keyboardBounds.ReduceFromLeft(100.f);
    pGraphics->AttachControl(new IVKeyboardControl(keyboardBounds, 24, 96, false), kCtrlTagKeyboard);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5)), kCtrlTagBender);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5, true), IMidiMsg::EControlChangeMsg::kModWheel));

    b = b.GetReducedFromBottom(137);
    const IRECT channelPanel = b.GetFromLeft(84);
    pGraphics->AttachControl(new IVToggleControl(channelPanel.GetGridCell(0,6,1).GetReducedFromBottom(16.f), kParamPulse1Enabled,   "Pulse 1",  style), kNoTag, "NES");
    pGraphics->AttachControl(new IVToggleControl(channelPanel.GetGridCell(1,6,1).GetReducedFromBottom(16.f), kParamPulse2Enabled,   "Pulse 2",  style), kNoTag, "NES");
    pGraphics->AttachControl(new IVToggleControl(channelPanel.GetGridCell(2,6,1).GetReducedFromBottom(16.f), kParamTriangleEnabled, "Triangle", style), kNoTag, "NES");
    pGraphics->AttachControl(new IVToggleControl(channelPanel.GetGridCell(3,6,1).GetReducedFromBottom(16.f), kParamNoiseEnabled,    "Noise",    style), kNoTag, "NES");
    pGraphics->AttachControl(new IVToggleControl(channelPanel.GetGridCell(4,6,1).GetReducedFromBottom(16.f), kParamDpcmEnabled,     "DPCM",     style), kNoTag, "NES");

    pGraphics->AttachControl(new IVButtonControl(channelPanel.GetGridCell(5,6,1), [pGraphics, this](IControl* pCaller){
      SplashClickActionFunc(pCaller);
      static IPopupMenu menu {"Menu", {"Pulse 1", "Pulse 2", "Triangle", "Noise", "DPCM"}, [pCaller, this](IPopupMenu* pMenu) {
          auto* itemChosen = pMenu->GetChosenItem();
          if(itemChosen) {
            pCaller->As<IVButtonControl>()->SetValueStr(itemChosen->GetText());
            switch(pMenu->GetChosenItemIdx()) {
              case 0: mDSP.SetActiveChannel(NesApu::Channel::Pulse1); break;
              case 1: mDSP.SetActiveChannel(NesApu::Channel::Pulse2); break;
              case 2: mDSP.SetActiveChannel(NesApu::Channel::Triangle); break;
              case 3: mDSP.SetActiveChannel(NesApu::Channel::Noise); break;
              case 4: mDSP.SetActiveChannel(NesApu::Channel::Dpcm); break;
            }

            if (pMenu->GetChosenItemIdx() == 4) { // DPCM
              GetUI()->GetControlWithTag(kCtrlTagDpcmEditor)->Hide(false);
              GetUI()->ForControlInGroup("StepSequencers", [](IControl& control) {
                control.Hide(true);
              });
              GetUI()->ForControlInGroup("Sliders", [](IControl& control) {
                control.Hide(true);
              });
            } else {
              GetUI()->GetControlWithTag(kCtrlTagDpcmEditor)->Hide(true);
              GetUI()->ForControlInGroup("StepSequencers", [](IControl& control) {
                control.Hide(false);
              });
              GetUI()->ForControlInGroup("Sliders", [](IControl& control) {
                control.Hide(false);
              });
            }

            // update all envelope values
            auto ss1 = dynamic_cast<StepSequencer*>(GetUI()->GetControlWithTag(kCtrlTagEnvelope1));
            const NesEnvelope* env1 = mDSP.mNesEnvelope1;
            for (int i = 0; i < 64; i++) {
              GetParam(kParamEnv1 + i)->Set(env1->mValues[i]);
            }
            ss1->SetLoopPoint(   env1->mLoopPoint);
            ss1->SetReleasePoint(env1->mReleasePoint);
            ss1->SetLength(      env1->mLength);

            auto ss2 = dynamic_cast<StepSequencer*>(GetUI()->GetControlWithTag(kCtrlTagEnvelope2));
            const NesEnvelope* env2 = mDSP.mNesEnvelope2;
            for (int i = 0; i < 64; i++) {
              GetParam(kParamEnv2 + i)->Set(env2->mValues[i]);
            }
            ss2->SetLoopPoint(   env2->mLoopPoint);
            ss2->SetReleasePoint(env2->mReleasePoint);
            ss2->SetLength(      env2->mLength);

            auto ss3 = dynamic_cast<StepSequencer*>(GetUI()->GetControlWithTag(kCtrlTagEnvelope3));
            const NesEnvelope* env3 = mDSP.mNesEnvelope3;
            for (int i = 0; i < 64; i++) {
              GetParam(kParamEnv3 + i)->Set(env3->mValues[i]);
            }
            ss3->SetLoopPoint(   env3->mLoopPoint);
            ss3->SetReleasePoint(env3->mReleasePoint);
            ss3->SetLength(      env3->mLength);

            auto ss4 = dynamic_cast<StepSequencer*>(GetUI()->GetControlWithTag(kCtrlTagEnvelope4));
            const NesEnvelope* env4 = mDSP.mNesEnvelope4;
            for (int i = 0; i < 64; i++) {
              GetParam(kParamEnv4 + i)->Set(env4->mValues[i]);
            }
            ss4->SetLoopPoint(   env4->mLoopPoint);
            ss4->SetReleasePoint(env4->mReleasePoint);
            ss4->SetLength(      env4->mLength);

            // update all env params sliders/knobs
            GetParam(kParamEnv1LoopPoint)->Set(env1->mLoopPoint);
            GetParam(kParamEnv2LoopPoint)->Set(env2->mLoopPoint);
            GetParam(kParamEnv3LoopPoint)->Set(env3->mLoopPoint);
            GetParam(kParamEnv4LoopPoint)->Set(env4->mLoopPoint);

            GetParam(kParamEnv1RelPoint)->Set(env1->mReleasePoint);
            GetParam(kParamEnv2RelPoint)->Set(env2->mReleasePoint);
            GetParam(kParamEnv3RelPoint)->Set(env3->mReleasePoint);
            GetParam(kParamEnv4RelPoint)->Set(env4->mReleasePoint);

            GetParam(kParamEnv1Length)->Set(env1->mLength);
            GetParam(kParamEnv2Length)->Set(env2->mLength);
            GetParam(kParamEnv3Length)->Set(env3->mLength);
            GetParam(kParamEnv4Length)->Set(env4->mLength);

            GetParam(kParamEnv1SpeedDiv)->Set(env1->mSpeedDivider);
            GetParam(kParamEnv2SpeedDiv)->Set(env2->mSpeedDivider);
            GetParam(kParamEnv3SpeedDiv)->Set(env3->mSpeedDivider);
            GetParam(kParamEnv4SpeedDiv)->Set(env4->mSpeedDivider);

            ss1->SetSlidersDirty();
            ss2->SetSlidersDirty();
            ss3->SetSlidersDirty();
            ss4->SetSlidersDirty();

            SendCurrentParamValuesFromDelegate();
          }
        }
      };
      float x, y;
      pGraphics->GetMouseDownPoint(x, y);
      pGraphics->CreatePopupMenu(*pCaller, menu, x, y);
    }, "EDIT", style, false, true), kNoTag, "vcontrols");

    const IRECT envelopesPanel = b.GetReducedFromLeft(100).GetPadded(8);

    auto createEnvelopePanel = [=](IRECT rect, const char* label, int envP, int lpP, int rpP, int lP, int sdP, int ctrlTag, IColor color) {
      auto stepSeq = new StepSequencer(rect.GetReducedFromBottom(40), label, style.WithColor(kFG, color).WithColor(kBG, IColor::FromColorCodeStr("#141414")), 1.f/(GetParam(envP)->GetRange()), envP, 64);
      pGraphics->AttachControl(stepSeq, ctrlTag, "StepSequencers");

      auto lpC = new IVSliderControl(rect.GetFromBottom(10).GetVShifted(-30), lpP, nullptr, noLabelStyle, true, EDirection::Horizontal, 1., 0, 5, false);
      lpC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetLoopPoint(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(lpC, kNoTag, "Sliders");
      stepSeq->SetLoopPoint(GetParam(kParamEnv1LoopPoint)->Int());

      auto rpC = new IVSliderControl(rect.GetFromBottom(10).GetVShifted(-20), rpP, nullptr, noLabelStyle, true, EDirection::Horizontal, 1., 0, 5, false);
      rpC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetReleasePoint(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(rpC, kNoTag, "Sliders");
      stepSeq->SetReleasePoint(GetParam(kParamEnv1RelPoint)->Int());

      auto lC =  new IVSliderControl(rect.GetFromBottom(10).GetVShifted(-10), lP, nullptr, noLabelStyle, true, EDirection::Horizontal, 1., 0, 5, false);
      lC->SetActionFunction([=](IControl *pCaller) {
        stepSeq->SetLength(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(lC, kNoTag, "Sliders");
      stepSeq->SetLength(GetParam(kParamEnv1Length)->Int());

      pGraphics->AttachControl(new IVSliderControl(rect.GetFromBottom(10).GetVShifted(0), sdP, nullptr, noLabelStyle, true, EDirection::Horizontal, 1., 0, 5, false), kNoTag, "Sliders");
    };

    createEnvelopePanel(envelopesPanel.GetGridCell(0,2,2).GetPadded(-8), "VOLUME",     kParamEnv1, kParamEnv1LoopPoint, kParamEnv1RelPoint, kParamEnv1Length, kParamEnv1SpeedDiv, kCtrlTagEnvelope1, IColor::FromColorCodeStr("#CC2626"));
    createEnvelopePanel(envelopesPanel.GetGridCell(1,2,2).GetPadded(-8), "DUTY",       kParamEnv2, kParamEnv2LoopPoint, kParamEnv2RelPoint, kParamEnv2Length, kParamEnv2SpeedDiv, kCtrlTagEnvelope2, IColor::FromColorCodeStr("#DE5E33"));
    createEnvelopePanel(envelopesPanel.GetGridCell(2,2,2).GetPadded(-8), "PITCH",      kParamEnv3, kParamEnv3LoopPoint, kParamEnv3RelPoint, kParamEnv3Length, kParamEnv3SpeedDiv, kCtrlTagEnvelope3, IColor::FromColorCodeStr("#53AD8E"));
    createEnvelopePanel(envelopesPanel.GetGridCell(3,2,2).GetPadded(-8), "FINE PITCH", kParamEnv4, kParamEnv4LoopPoint, kParamEnv4RelPoint, kParamEnv4Length, kParamEnv4SpeedDiv, kCtrlTagEnvelope4, IColor::FromColorCodeStr("#747ACD"));


    auto dpcmEditor = new IDpcmEditorControl(envelopesPanel, style, mDSP.mNesChannels->dpcm.mNesDpcm);
    pGraphics->AttachControl(dpcmEditor, kCtrlTagDpcmEditor, "DpcmEditor");
    dpcmEditor->Hide(true);

# pragma mark - Presets


    MakePresetFromNamedParams("Basic Square",   1, kParamPulse1Enabled,   1);
    MakePresetFromNamedParams("Basic Triangle", 1, kParamTriangleEnabled, 1);
    MakePresetFromNamedParams("Basic Noise",    1, kParamNoiseEnabled,    1);
    MakePresetFromNamedParams("Basic DPCM",     1, kParamDpcmEnabled,     1);

    pGraphics->AttachControl(new IVBakedPresetManagerControl(IRECT{0,0,300,40}, style)); // "./presets", "nesvst"));
    pGraphics->AttachControl(new IVButtonControl(IRECT{300,0,350,40}, [this](IControl* control) {
//      IByteCh
//      SerializeParams();
//      DumpPresetBlob("presets", );
      const char** x{};
      DumpPresetSrcCode("presets.c", x);
    }, "Save", style));

//    pGraphics->AttachControl(new IVButtonControl(keyboardBounds.GetFromTRHC(200, 30).GetTranslated(0, -30), SplashClickActionFunc,
//      "Show/Hide Keyboard", DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE).WithLabelText({15.f, EVAlign::Middle})))->SetAnimationEndActionFunction(
//      [pGraphics](IControl* pCaller) {
//        static bool hide = false;
//        pGraphics->GetControlWithTag(kCtrlTagKeyboard)->Hide(hide = !hide);
//        pGraphics->Resize(PLUG_WIDTH, hide ? PLUG_HEIGHT / 2 : PLUG_HEIGHT, pGraphics->GetDrawScale());
//    });


    pGraphics->SetQwertyMidiKeyHandlerFunc([pGraphics](const IMidiMsg &msg) {
      dynamic_cast<IVKeyboardControl *>(
        pGraphics->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(
          msg.NoteNumber(), msg.StatusMsg() == IMidiMsg::kNoteOn);
    });

  };
#endif
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
  return IPluginBase::SerializeState(chunk);
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
