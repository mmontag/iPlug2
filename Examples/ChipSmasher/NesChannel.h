//
//  NesChannelState.h
//  All
//
//  Created by Matt Montag on 5/20/20.
//

#ifndef NesChannelState_h
#define NesChannelState_h

#include "NesApu.h"
#include "NesDpcm.h"
#include "NesEnvelope.h"
#include <algorithm>
#include <utility>

using namespace std;

class NesChannel
{
public:
  NesChannel(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes)
  : mNesApu(std::move(nesApu))
  , mEnvs(nesEnvelopes)
  , mChannel(channel)
  , mNoteTable(NesApu::GetNoteTableForChannel(channel))
  // TODO: pal, numN163Channels
  {}

  virtual int GetPeriod() {
    int arpNote = mEnvs.arp.GetValueAndAdvance();
    // TODO: scaled fine pitch mode so that fine pitch isn't useless for low notes
    int basePeriod = mNoteTable[mBaseNote + arpNote] - mEnvs.pitch.GetValueAndAdvance();
    int period = clamp(basePeriod / mPitchBendRatio, 8, 2047);
    return period;
  }

  virtual int GetVolume() {
    int envVolume = mEnvs.volume.GetValueAndAdvance();
    // Simple multiply https://docs.google.com/spreadsheets/d/1i1xJdoUZuDM50SogPGg270OP6oX1rjiDNVBMh6yfQiw/edit#gid=1871770382
    return ceil(envVolume * mVelocity);
  }

  virtual int GetDuty() {
    return mEnvs.duty.GetValueAndAdvance() % 4; // 2A03 pulse channels duty: 0, 1, 2, 3
  }

  // TODO rename something like Advance() or EndFrame()
  virtual void UpdateAPU() {
  }

  virtual void SetPitchBend(float pitchBend) {
    if (mPitchBend != pitchBend) {
      mPitchBendRatio = pow(2., pitchBend);
      mPitchBend = pitchBend;
    }
  }

  virtual void Trigger(int baseNote, double velocity) {
    mBaseNote = baseNote;
    mVelocity = velocity;
    mEnvs.volume.Trigger();
    mEnvs.arp.Trigger();
    mEnvs.pitch.Trigger();
    mEnvs.duty.Trigger();
  }

  virtual void Release() {
    mEnvs.volume.Release();
    mEnvs.arp.Release();
    mEnvs.pitch.Release();
    mEnvs.duty.Release();
  }

  virtual void Serialize(iplug::IByteChunk &chunk) {
    for (auto env : mEnvs.allEnvs) {
      env->Serialize(chunk);
    }
  }

  virtual int Deserialize(const iplug::IByteChunk &chunk, int startPos) {
    int pos = startPos;
    for (auto env : mEnvs.allEnvs) {
      pos = env->Deserialize(chunk, pos);
    }
    return pos;
  }

  //protected:
  shared_ptr<Simple_Apu> mNesApu;
  NesApu::Channel mChannel;
  array<ushort, 97> mNoteTable;
  int mBaseNote = 40;
  NesEnvelopes mEnvs;
  float mPitchBendRatio = 1;
  float mPitchBend = 0;
  float mVelocity;
};

class NesChannelPulse : public NesChannel
{
public:
  int mRegOffset = 0;
  int mPrevPeriodHi = 1000;

  NesChannelPulse(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes)
  {
    // 0x4000 for Square1, 0x4004 for Square2
    mRegOffset = mChannel * 4;
  }

  virtual void UpdateAPU() override {
    int duty = GetDuty();
    int volume = 0;

    if (mEnvs.arp.GetState() != NesEnvelope::ENV_OFF) {
      int period = GetPeriod();
      volume = GetVolume();

      int periodLo = (period >> 0) & 0xff;
      int periodHi = (period >> 8) & 0x07;
      int deltaHi = periodHi - mPrevPeriodHi;

      if (deltaHi != 0) {
        // TODO: verify sweep is working, and get smoothVibrato from some setting
        bool smoothVibrato = true;
        if (smoothVibrato && abs(deltaHi) == 1) { // originally && !IsSeeking()
          // Blaarg's smooth vibrato technique using the sweep
          // to avoid resetting the phase. Cool stuff.
          // http://forums.nesdev.com/viewtopic.php?t=231

          // reset frame counter in case it was about to clock
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0x40);
          // be sure low 8 bits of timer period are $FF ($00 when negative)
          mNesApu->write_register(NesApu::APU_PL1_LO + mRegOffset, deltaHi < 0 ? 0x00 : 0xff);
          // sweep enabled, shift = 7, set negative flag.
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + mRegOffset, deltaHi < 0 ? 0x8f : 0x87);
          // clock sweep immediately
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0xc0);
          // disable sweep
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + mRegOffset, 0x08);
        } else {
          mNesApu->write_register(NesApu::APU_PL1_HI + mRegOffset, periodHi);
        }

        mPrevPeriodHi = periodHi;
      }

      mNesApu->write_register(NesApu::APU_PL1_LO + mRegOffset, periodLo);
    }

    // duty is shifted to 2 most significant bits
    mNesApu->write_register(NesApu::APU_PL1_VOL + mRegOffset, (duty << 6) | 0x30 | volume);
    NesChannel::UpdateAPU();
  }
};

class NesChannelTriangle : public NesChannel
{
public:
  NesChannelTriangle(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes) {}

  virtual int GetVolume() {
    return mEnvs.volume.GetValueAndAdvance() ? 0xff : 0x80;
  }

  virtual void UpdateAPU() {
    if (mEnvs.volume.GetState() != NesEnvelope::ENV_OFF) {
      int volume = GetVolume();
      int period = GetPeriod();
      int periodLo = (period >> 0) & 0xff;
      int periodHi = (period >> 8) & 0x07;

      mNesApu->write_register(NesApu::APU_TRI_LO, periodLo);
      mNesApu->write_register(NesApu::APU_TRI_HI, periodHi);
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0x80 | volume);
    } else {
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0x80);
    }

    NesChannel::UpdateAPU();
  }
};

class NesChannelNoise : public NesChannel
{
public:
  NesChannelNoise(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes) {}

  virtual int GetPeriod() {
    return (mBaseNote + mEnvs.arp.GetValueAndAdvance()) & 0x0f;
  }

  virtual void UpdateAPU() {
    if (mEnvs.volume.GetState() != NesEnvelope::ENV_OFF) {
      int volume = GetVolume();
      int duty = GetDuty();
      int period = GetPeriod();

      mNesApu->write_register(NesApu::APU_NOISE_LO, (period  ^ 0x0f) | ((duty << 7) & 0x80));
      mNesApu->write_register(NesApu::APU_NOISE_VOL, 0xf0 | volume);
    } else {
      mNesApu->write_register(NesApu::APU_NOISE_VOL, 0xf0);
    }

    NesChannel::UpdateAPU();
  }
};

class NesChannelDpcm : public NesChannel
{
public:
  NesChannelDpcm(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, shared_ptr<NesDpcm> nesDpcm)
  : NesChannel(nesApu, channel, NesEnvelopes())
  , mNesDpcm(std::move(nesDpcm))
  {}

  virtual void Trigger(int baseNote, double velocity) {
    mBaseNote = baseNote;
    mDpcmTriggered = true;
  }

  virtual void Release() {
    mDpcmReleased = true;
  }

  virtual void UpdateAPU() {
    if (mDpcmTriggered) {
      mDpcmTriggered = false;
      mNesApu->write_register(NesApu::APU_SND_CHN, 0x0f);

      auto patch = mNesDpcm->GetDpcmPatchForNote(mBaseNote);

      if (patch && patch->sampleIdx > -1) {
        auto sample = mNesDpcm->mSamples[patch->sampleIdx];
        /// $4012 AAAA.AAAA
        /// Sample address = %11AAAAAA.AA000000 = $C000 + (A * 64)
        mNesApu->write_register(NesApu::APU_DMC_START, mNesDpcm->GetAddressForSample(sample) / 64); // >> 6
        /// $4013 LLLL.LLLL
        /// Sample length = %0000LLLL.LLLL0001 = (L * 16) + 1 bytes.
        /// Specify the length of the sample in 16 byte increments by writing a value to $4013.
        /// i.e. $01 means 17 bytes length and $02 means 33 bytes.
        /// For an actual length of 513 bytes, write 32 to $4013. (513 - 1)/16 = 32
        mNesApu->write_register(NesApu::APU_DMC_LEN, sample->length() >> 4); // >> 4
        mNesApu->write_register(NesApu::APU_DMC_FREQ, patch->pitch | (patch->loop ? 0x40 /* 0100 0000 */ : 0x00));
        mNesApu->write_register(NesApu::APU_DMC_RAW, 32); // Starting sample
        mNesApu->write_register(NesApu::APU_SND_CHN, 0x1f); // 0001 1111
      }
    }
    if (mDpcmReleased) {
      mDpcmReleased = false;
      mNesApu->write_register(NesApu::APU_SND_CHN, 0x0f);
    }

    NesChannel::UpdateAPU();
  }

  void Serialize(iplug::IByteChunk &chunk) override {
    mNesDpcm->Serialize(chunk);
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos) override {
    return mNesDpcm->Deserialize(chunk, startPos);
  }

  shared_ptr<NesDpcm> mNesDpcm;
protected:
  bool mDpcmTriggered;
  bool mDpcmReleased;
};



struct NesChannels {
  explicit NesChannels(NesChannelPulse p1, NesChannelPulse p2, NesChannelTriangle t, NesChannelNoise n, NesChannelDpcm d)
    : pulse1(std::move(p1))
    , pulse2(std::move(p2))
    , triangle(std::move(t))
    , noise(std::move(n))
    , dpcm(std::move(d))
    , allChannels({&pulse1, &pulse2, &triangle, &noise, &dpcm}) {}

  NesChannelPulse pulse1;
  NesChannelPulse pulse2;
  NesChannelTriangle triangle;
  NesChannelNoise noise;
  NesChannelDpcm dpcm;

  vector<NesChannel*> allChannels;
};

#endif /* NesChannelState_h */
