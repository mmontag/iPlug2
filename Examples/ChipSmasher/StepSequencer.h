//
//  StepSequencer.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/27/20.
//

#ifndef StepSequencer_h
#define StepSequencer_h


#include "IControl.h"

/** A base class for mult-strip/track controls, such as multi-sliders, meters */
class StepSequencerBase : public IControl
                         , public IVectorBase
{
public:
  StepSequencerBase(const IRECT& bounds, const char* label, const IVStyle& style, int maxNTracks = 1, EDirection dir = EDirection::Horizontal, float minTrackValue = 0.f, float maxTrackValue = 1.f, std::initializer_list<const char*> trackNames = {})
  : IControl(bounds)
  , IVectorBase(style)
  , mMinTrackValue(minTrackValue)
  , mMaxTrackValue(maxTrackValue)
  , mDirection(dir)
  {
    SetNVals(maxNTracks);

    for (int i=0; i<maxNTracks; i++)
    {
      SetParamIdx(kNoParameter, i);
      mTrackBounds.Add(IRECT());
    }

    if(trackNames.size())
    {
      assert(trackNames.size() == maxNTracks); // check that the trackNames list size matches the number of tracks

      for (auto& trackName : trackNames)
      {
        mTrackNames.Add(new WDL_String(trackName));
      }
    }

    AttachIControl(this, label);
  }

  StepSequencerBase(const IRECT& bounds, const char* label, const IVStyle& style, int lowParamidx, int maxNTracks = 1, EDirection dir = EDirection::Horizontal, float minTrackValue = 0.f, float maxTrackValue = 1.f, float grain = 0.001f, std::initializer_list<const char*> trackNames = {})
  : IControl(bounds)
  , IVectorBase(style)
  , mMinTrackValue(minTrackValue)
  , mMaxTrackValue(maxTrackValue)
  , mDirection(dir)
  , mGrain(grain)
  {
    SetNVals(maxNTracks);

    for (int i = 0; i < maxNTracks; i++)
    {
      SetParamIdx(lowParamidx+i, i);
      mTrackBounds.Add(IRECT());
    }

    if(trackNames.size())
    {
      assert(trackNames.size() == maxNTracks);

      for (auto& trackName : trackNames)
      {
        mTrackNames.Add(new WDL_String(trackName));
      }
    }

    AttachIControl(this, label);
  }

  StepSequencerBase(const IRECT& bounds, const char* label, const IVStyle& style, const std::initializer_list<int>& params, EDirection dir = EDirection::Horizontal, float minTrackValue = 0.f, float maxTrackValue = 1.f, std::initializer_list<const char*> trackNames = {})
  : IControl(bounds)
  , IVectorBase(style)
  , mMinTrackValue(minTrackValue)
  , mMaxTrackValue(maxTrackValue)
  , mDirection(dir)
  {
    SetNVals(static_cast<int>(params.size()));

    int valIdx = 0;
    for (auto param : params)
    {
      SetParamIdx(param, valIdx++);
      mTrackBounds.Add(IRECT());
    }


    if(trackNames.size())
    {
      assert(trackNames.size() == params.size());

      for (auto& trackName : trackNames)
      {
        mTrackNames.Add(new WDL_String(trackName));
      }
    }

    AttachIControl(this, label);
  }

  virtual ~StepSequencerBase()
  {
    mTrackNames.Empty(true);
  }

  virtual void MakeTrackRects(const IRECT& bounds)
  {
    int nVals = NVals();
    int dir = static_cast<int>(mDirection); // 0 = horizontal, 1 = vertical
    for (int ch = 0; ch < nVals; ch++)
    {
      mTrackBounds.Get()[ch] = bounds.SubRect(EDirection(!dir), nVals, ch).
                                     GetPadded(0, -mTrackPadding * (float) dir, -mTrackPadding * (float) !dir, -mTrackPadding);
    }
  }

  void DrawWidget(IGraphics& g) override
  {
    int nVals = NVals();

    // Draw grid lines
    float nSteps = (mMaxTrackValue - mMinTrackValue)/mGrain;
    IRECT line = mWidgetBounds.GetFromTop(1);
    float gapHeight = mWidgetBounds.H() / nSteps;
    for (int i = 1; i < nSteps; i++) {
      g.FillRect(GetColor(kX1), line.GetTranslated(0.f, i * gapHeight), &mBlend);;
    }

    // Draw tracks
    for (int ch = 0; ch < nVals; ch++)
    {
      DrawTrack(g, mTrackBounds.Get()[ch], ch);
    }
  }

  /** Update the parameters based on a parameter group name.
   * You probably want to call \c IEditorDelegate::SendCurrentParamValuesFromDelegate() after this, to update the control values */
  void SetParamsByGroup(const char* paramGroup)
  {
    int nParams = GetDelegate()->NParams();
    std::vector<int> paramIdsForGroup;

    for (auto p = 0; p < nParams; p++)
    {
      IParam* pParam = GetDelegate()->GetParam(p);

      if(strcmp(pParam->GetGroup(), paramGroup) == 0)
      {
        paramIdsForGroup.push_back(p);
      }
    }

    mTrackBounds.Resize(0);

    int nParamsInGroup = static_cast<int>(paramIdsForGroup.size());

    SetNVals(nParamsInGroup);

    int valIdx = 0;
    for (auto param : paramIdsForGroup)
    {
      SetParamIdx(param, valIdx++);
      mTrackBounds.Add(IRECT());
    }

    OnResize();
  }

  /** Update the parameters based on a parameter group name.
   * You probably want to call IEditorDelegate::SendCurrentParamValuesFromDelegate() after this, to update the control values */
  void SetParams(std::initializer_list<int> paramIds)
  {
    mTrackBounds.Resize(0);

    SetNVals(static_cast<int>(paramIds.size()));

    int valIdx = 0;
    for (auto param : paramIds)
    {
      SetParamIdx(param, valIdx++);
      mTrackBounds.Add(IRECT());
    }

    OnResize();
  }

  bool HasTrackNames() const
  {
    return mTrackNames.GetSize() > 0;
  }

  const char* GetTrackName(int chIdx) const
  {
    WDL_String* pStr = mTrackNames.Get(chIdx);
    return pStr ? pStr->Get() : "";
  }

  void SetTrackName(int chIdx, const char* newName)
  {
    assert(chIdx >= 0 && chIdx < mTrackNames.GetSize());

    if(chIdx >= 0 && chIdx < mTrackNames.GetSize())
    {
      mTrackNames.Get(chIdx)->Set(newName);
    }
  }
//
//  void SetHighlightIdx(int chIdx) {
//    chIdx = clamp(chIdx, -1, NVals());
//
//    if (mHighlightIdx != chIdx) {
//      mHighlightIdx = chIdx;
//      SetDirty(false, chIdx);
//    }
//  }

protected:

  virtual void DrawTrack(IGraphics& g, const IRECT& r, int chIdx)
  {
    DrawTrackBG(g, r, chIdx);
//
//    if(chIdx == mHighlightIdx)
//      g.FillRect(COLOR_RED, r, &BLEND_50);

    if(HasTrackNames())
      DrawTrackName(g, r, chIdx);

    DrawTrackHandle(g, r, chIdx);

    if(mStyle.drawFrame && mDrawTrackFrame)
      g.DrawRect(GetColor(kFR), r, &mBlend, mStyle.frameThickness);
  }

  virtual void DrawTrackBG(IGraphics& g, const IRECT& r, int chIdx)
  {
    g.FillRect(kBG, r, &mBlend);
  }

  virtual void DrawTrackName(IGraphics& g, const IRECT& r, int chIdx)
  {
    g.DrawText(mText, GetTrackName(chIdx), r);
  }

  virtual void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx)
  {
    IRECT fillRect = r.FracRect(mDirection, static_cast<float>(GetValue(chIdx)));

    g.FillRect(GetColor(kFG), fillRect, &mBlend); // TODO: shadows!

    IRECT peakRect;

    if(mDirection == EDirection::Vertical)
      peakRect = IRECT(fillRect.L, fillRect.T, fillRect.R, fillRect.T + mPeakSize);
    else
      peakRect = IRECT(fillRect.R - mPeakSize, fillRect.T, fillRect.R, fillRect.B);

    DrawPeak(g, peakRect, chIdx);
  }

  virtual void DrawPeak(IGraphics& g, const IRECT& r, int chIdx)
  {
    g.FillRect(GetColor(kFR), r, &mBlend);
  }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    MakeTrackRects(mWidgetBounds);
    SetDirty(false);
  }

protected:
  EDirection mDirection = EDirection::Vertical;
  WDL_TypedBuf<IRECT> mTrackBounds;
  WDL_PtrList<WDL_String> mTrackNames;
  float mMinTrackValue;
  float mMaxTrackValue;
  float mTrackPadding = 0.;
  float mPeakSize = 1.;
  float mGrain = 0.001f;
  bool mDrawTrackFrame = true;
  int mHighlightIdx = -1;
};

//BEGIN_IPLUG_NAMESPACE
//BEGIN_IGRAPHICS_NAMESPACE

/** A vectorial multi-slider control
 * @ingroup IControls */
template <int MAXNC = 1>
class StepSequencer : public StepSequencerBase
{
public:

  /** Constructs a vector multi slider control that is not linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param direction The direction of the sliders
   * @param minTrackValue Defines the minimum value of each slider
   * @param maxTrackValue Defines the maximum value of each slider */
  StepSequencer(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, float grain = 0.001f, int loParamIdx = -1, EDirection dir = EDirection::Vertical, float minTrackValue = 0.f, float maxTrackValue = 1.f)
  : StepSequencerBase(bounds, label, style, loParamIdx, MAXNC, dir, minTrackValue, maxTrackValue, grain)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  /** Constructs a vector multi slider control that is linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param loParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param direction The direction of the sliders
   * @param minTrackValue Defines the minimum value of each slider
   * @param maxTrackValue Defines the maximum value of each slider */
  StepSequencer(const IRECT& bounds, const char* label, const IVStyle& style, int loParamIdx, EDirection dir, float minTrackValue, float maxTrackValue) //FIXME: float minTrackValue, float maxTrackValue?
  : StepSequencerBase(bounds, label, style, loParamIdx, MAXNC, dir, minTrackValue, maxTrackValue)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  StepSequencer(const IRECT& bounds, const char* label, const IVStyle& style, const std::initializer_list<int>& params, EDirection dir, float minTrackValue, float maxTrackValue)//, const char* trackNames = 0, ...)
  : StepSequencerBase(bounds, label, style, params, dir, minTrackValue, maxTrackValue)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  int GetValIdxForPos(float x, float y) const override
  {
    int nVals = NVals();

    for (auto v = 0; v < nVals; v++)
    {
      if (mTrackBounds.Get()[v].Contains(x, y))
      {
        return v;
      }
    }

    return kNoValIdx;
  }

  void SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, double minClip = 0., double maxClip = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = NVals();

    float value = 0.;
    int sliderTest = -1;

    if(direction == EDirection::Vertical)
    {
      value = 1.f - (y-bounds.T) / bounds.H();

      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].Contains(x, mTrackBounds.Get()[i].MH()))
        {
          sliderTest = i;
          break;
        }
      }
    }
    else
    {
      value = (x-bounds.L) / bounds.W();

      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].Contains(mTrackBounds.Get()[i].MW(), y))
        {
          sliderTest = i;
          break;
        }
      }
    }

    value = std::ceil( value / mGrain ) * mGrain;

    if (sliderTest > -1)
    {
      mSliderHit = sliderTest;

      float oldValue = GetValue(sliderTest);
      float newValue = mMinTrackValue + Clip(value, 0.f, 1.f) * (mMaxTrackValue - mMinTrackValue);
      if (newValue != oldValue) {
        SetValue(newValue, sliderTest);
        OnNewValue(sliderTest, newValue);
        SetDirty(true, mSliderHit); // will send all param vals parameter value to delegate
      }

      if (mPrevSliderHit != -1)
      {
        if (abs(mPrevSliderHit - mSliderHit) > 1 /*|| shiftClicked*/)
        {
          int lowBounds, highBounds;

          if (mPrevSliderHit < mSliderHit)
          {
            lowBounds = mPrevSliderHit;
            highBounds = mSliderHit;
          }
          else
          {
            lowBounds = mSliderHit;
            highBounds = mPrevSliderHit;
          }

          for (auto i = lowBounds; i < highBounds; i++)
          {
            double frac = (double)(i - lowBounds) / double(highBounds-lowBounds);

            float oldValue = GetValue(i);
            float newValue = std::ceil(iplug::Lerp(GetValue(lowBounds), GetValue(highBounds), frac) / mGrain) * mGrain;
            if (newValue != oldValue) {
              SetValue(newValue, i);
              OnNewValue(i, newValue);
              SetDirty(true, i);
            }
          }
        }
      }
      mPrevSliderHit = mSliderHit;
    }
    else
    {
      mSliderHit = -1;
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (!mod.S)
      mPrevSliderHit = -1;

    SnapToMouse(x, y, mDirection, mWidgetBounds);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    SnapToMouse(x, y, mDirection, mWidgetBounds);
  }

  //override to do something when an individual slider is dragged
  virtual void OnNewValue(int trackIdx, double val) {}

protected:
  int mPrevSliderHit = -1;
  int mSliderHit = -1;
};


#endif /* StepSequencer_h */