//
//  StepSequencer.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/27/20.
//

#ifndef StepSequencer_h
#define StepSequencer_h


#include "IControl.h"

using StepSeqFunc = std::function<void(int step, int value)>;

/** A base class for mult-strip/slider controls, such as multi-sliders, meters */
class StepSequencer : public IControl, public IVectorBase {
public:

  /** Constructs a vector multi slider control that is linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param grain The smallest value increment of the sliders
   * @param lowParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param direction The direction of the sliders
   * @param minSliderValue Defines the minimum value of each slider
   * @param maxSliderValue Defines the maximum value of each slider */
  StepSequencer(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, float grain = 0.001f, int lowParamIdx = -1, int maxNSliders = 1, EDirection dir = EDirection::Vertical, float minSliderValue = 0.f, float maxSliderValue = 1.f)
  : IControl(bounds)
  , IVectorBase(style)
  , mMinSliderValue(minSliderValue)
  , mMaxSliderValue(maxSliderValue)
  , mDirection(dir)
  , mGrain(grain)
  , mLength(maxNSliders)
  {
    mSliderPadding = 1.f;

    SetNVals(maxNSliders);

    for (int i = 0; i < maxNSliders; i++)
    {
      SetParamIdx(lowParamIdx + i, i); // or kNoParameter
      mSliderBounds.Add(IRECT());
    }

    AttachIControl(this, label);
  }

  virtual ~StepSequencer()
  {
  }

  IRECT MakeRects(const IRECT& parent, bool hasHandle = false) {
    IVectorBase::MakeRects(parent, hasHandle);
    mLoopInfoBounds = mWidgetBounds.GetPadded(-3.0f).GetFromTop(4.0f);
    mInsetBounds = mWidgetBounds.GetPadded(-3.0f).GetReducedFromTop(8.0f);

    return mWidgetBounds;
  }

  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mWidgetBounds);
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
      if (mSliderBounds.Get()[v].Contains(x, y))
      {
        return v;
      }
    }

    return kNoValIdx;
  }

  virtual void MakeSliderRects(const IRECT& bounds)
  {
    int nVals = NVals();
    int dir = static_cast<int>(mDirection); // 0 = horizontal, 1 = vertical
    for (int ch = 0; ch < nVals; ch++)
    {
      mSliderBounds.Get()[ch] = bounds.SubRect(EDirection(!dir), mLength, ch).
                                     GetPadded(0, -mSliderPadding * (float) dir, -mSliderPadding * (float) !dir, -mSliderPadding);
    }
  }

  void SetLength(int length) {
    mLength = max(1, length);
    MakeSliderRects(mInsetBounds);
    SetSlidersDirty();
    SetDirty(false);
  }

  void SetLoopPoint(int idx) {
    mLoopPoint = idx;
    SetSlidersDirty();
    SetDirty(false);
  }

  void SetReleasePoint(int idx) {
    mReleasePoint = idx;
    SetSlidersDirty();
    SetDirty(false);
  }

  void SetSlidersDirty() {
    if (mLayerSliders) mLayerSliders->Invalidate();
  }

//  int ctr = 0;

  void DrawWidget(IGraphics& g) override
  {
//    if (ctr % 10 == 0)
//      DBGMSG("Drawing StepSequencer %d\n", ctr);
//    ctr++;

    if (!g.CheckLayer(mLayerGrid)) {
      g.StartLayer(this, mInsetBounds);

      // Draw grid lines
      float nSteps = (mMaxSliderValue - mMinSliderValue)/mGrain;
      IRECT line = mInsetBounds.GetFromTop(1);
      float gapHeight = mInsetBounds.H() / nSteps;
      for (int i = 1; i < nSteps; i++) {
        g.FillRect(COLOR_WHITE.WithOpacity(0.1), line.GetTranslated(0.f, i * gapHeight), &mBlend);;
      }

      mLayerGrid = g.EndLayer();
    }

    if(!g.CheckLayer(mLayerSliders)) {
      g.StartLayer(this, mInsetBounds);

      // Draw sliders
      for (int ch = 0; ch < mLength; ch++) {
        DrawSlider(g, mSliderBounds.Get()[ch], ch);
      }

      mLayerSliders = g.EndLayer();
    }

//    if (!g.CheckLayer(mLayerPlayhead)) {
//      IRECT r = mSliderBounds.Get()[0];
//      g.StartLayer(this, r);
//      g.FillRect(COLOR_RED, r, &BLEND_50);
//      mLayerPlayhead = g.EndLayer();
//    }

    g.DrawBitmap(mLayerGrid->GetBitmap(), mInsetBounds);

//    IRECT r = mSliderBounds.Get()[0];
//    g.DrawBitmap(mLayerPlayhead->GetBitmap(), mInsetBounds.GetHShifted(r.W() * mHighlightIdx));
    g.DrawBitmap(mLayerSliders->GetBitmap(), mInsetBounds);

    g.FillRect(COLOR_WHITE.WithOpacity(0.1), mLoopInfoBounds);
    IRECT loopRect = mLoopInfoBounds.GetFromRight((mLength - mLoopPoint   ) * mLoopInfoBounds.W() / mLength);
    IRECT relsRect = mLoopInfoBounds.GetFromRight((mLength - mReleasePoint) * mLoopInfoBounds.W() / mLength);
    IColor c1 = GetColor(kFG);
    IColor c2 = IColor::LinearInterpolateBetween(c1, COLOR_BLACK, 0.33);
    g.FillRect(c1, loopRect, &mBlend);
    g.FillRect(c2, relsRect, &mBlend);

    if (mHighlightIdx >= 0 && mHighlightIdx < mLength) {
      IRECT r = mSliderBounds.Get()[mHighlightIdx];
      g.FillRect(COLOR_WHITE, r, &BLEND_25);
    }

  }

  void SetHighlightIdx(int chIdx) {
    if (mHighlightIdx != chIdx) {
      mHighlightIdx = chIdx;
      SetDirty(false, clamp(chIdx, 0, NVals() - 1));
    }
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage) {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<1, int> d;
      pos = stream.Get(&d, pos);
      SetHighlightIdx(d.vals[0]);
    }
  }

  
  void SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, double minClip = 0., double maxClip = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = NVals();

    float value = 0.;
    int sliderTest = -1;

    if (direction == EDirection::Vertical) {
      value = 1.f - (y - bounds.T) / bounds.H();

      for (auto i = 0; i < nVals; i++) {
        if (mSliderBounds.Get()[i].Contains(x, mSliderBounds.Get()[i].MH())) {
          sliderTest = i;
          break;
        }
      }
    } else {
      value = (x - bounds.L) / bounds.W();

      for (auto i = 0; i < nVals; i++) {
        if (mSliderBounds.Get()[i].Contains(mSliderBounds.Get()[i].MW(), y)) {
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
      float newValue = mMinSliderValue + Clip(value, 0.f, 1.f) * (mMaxSliderValue - mMinSliderValue);
      if (newValue != oldValue) {
        mLayerSliders->Invalidate();
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
              mLayerSliders->Invalidate();
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

    SnapToMouse(x, y, mDirection, mInsetBounds);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    SnapToMouse(x, y, mDirection, mInsetBounds);
  }

  //override to do something when an individual slider is dragged
  virtual void OnNewValue(int sliderIdx, double val) {}

  virtual void DrawSlider(IGraphics& g, const IRECT& r, int chIdx)
  {
    IRECT fillRect = r.FracRect(mDirection, static_cast<float>(GetValue(chIdx)));
    g.FillRect(GetColor(kFG), fillRect, &mBlend);
  }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    MakeSliderRects(mInsetBounds);
    SetDirty(false);
  }

protected:
  EDirection mDirection = EDirection::Vertical;
  WDL_TypedBuf<IRECT> mSliderBounds;
  IRECT mInsetBounds;
  IRECT mLoopInfoBounds;
  float mMinSliderValue;
  float mMaxSliderValue;
  float mSliderPadding = 0.;
  float mGrain = 0.001f;
  int mHighlightIdx = -1;
  int mLoopPoint = 0;
  int mReleasePoint = 0;
  int mLength = 1;

  ILayerPtr mLayerGrid;
  ILayerPtr mLayerPlayhead;
  ILayerPtr mLayerSliders;

  int mPrevSliderHit = -1;
  int mSliderHit = -1;
};

//BEGIN_IPLUG_NAMESPACE
//BEGIN_IGRAPHICS_NAMESPACE

#endif /* StepSequencer_h */
