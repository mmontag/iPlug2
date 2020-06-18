//
//  NesEnvelope.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/27/20.
//

#ifndef NesEnvelope_h
#define NesEnvelope_h

#include "ISender.h"

const int MAX_STEPS = 64;

class NesEnvelope {
public:
  enum State {
    ENV_INITIAL,
    ENV_RELEASE,
    ENV_OFF
  };

  NesEnvelope(int defaultValue) {
    mValues.fill(defaultValue);
  }

  NesEnvelope(shared_ptr<array<int, 64>> values) : mNesEnvValues(values) {}

  void Trigger() {
    mState = ENV_INITIAL;
    mStep = 0;
  }

  void Release() {
    mStep = mReleasePoint * mSpeedDivider;
    if (mReleasePoint < mLength) {
      mState = ENV_RELEASE;
    } else {
      mState = ENV_OFF;
    }
  }

  int GetValueAndAdvance() {
    int step = mStep / mSpeedDivider;
    mStep++;
    switch (mState) {
      case ENV_INITIAL:
        if (mStep >= mReleasePoint * mSpeedDivider)
          mStep = mLoopPoint * mSpeedDivider;
      // break;
      // Fall through
      case ENV_RELEASE:
        if (mStep >= mLength * mSpeedDivider)
          mState = ENV_OFF;
        break;
      case ENV_OFF:
        return 0;
    }
    assert(step < mValues.size());
    return mValues.at(step);
  }

  int GetStep() {
    return mState == ENV_OFF ? -1 : mStep / mSpeedDivider;
  }

  State GetState() {
    return mState;
  }

  void SetLength(int length) {
    mLength = clamp(length, 1, 64);
//    if (mReleasePoint > mLength) mReleasePoint = mLength;
//    if (mLoopPoint > mLength) mLoopPoint = mLength;
  }

  void SetSpeedDivider(int speedDivider) {
    int newSpeedDivider = clamp(speedDivider, 1, 8);
    mStep = min(mStep * (float)newSpeedDivider / mSpeedDivider, 64 * newSpeedDivider - 1);
    mSpeedDivider = newSpeedDivider;
  }

  void SetLoop(int loopPoint) {
    mLoopPoint = clamp(loopPoint, 0, 64);
    if (mReleasePoint < mLoopPoint) mReleasePoint = mLoopPoint;
  }

  void SetRelease(int releasePoint) {
    mReleasePoint = clamp(releasePoint, 1, 64);
    if (mLoopPoint > mReleasePoint) mLoopPoint = mReleasePoint;
  }
  array<int, 64> mValues;

  int mStep = 0;
  int mLoopPoint = 15;
  int mReleasePoint = 16;
  int mLength = 16;
  int mSpeedDivider = 1;

protected:
  NesEnvelope::State mState = ENV_OFF;
  shared_ptr<array<int, 64>> mNesEnvValues;
};

struct NesEnvelopes {
  NesEnvelope arp{0};
  NesEnvelope pitch{0};
  NesEnvelope volume{15};
  NesEnvelope duty{2};
};

#endif /* NesEnvelope_h */
