//
//  NesChannelState.h
//  All
//
//  Created by Matt Montag on 5/20/20.
//

#ifndef NesChannelState_h
#define NesChannelState_h

#include "NesApu.h"
#include "NesEnvelope.h"
#include <algorithm>

using namespace std;

struct Note {
  int value;
  bool isStop;
  bool isMusical;
  bool isRelease;
};

class NesChannel
{
public:
  NesChannel(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, shared_ptr<NesEnvelope> nesEnvelope) // TODO: pal, numN163Channels
  {
    mNesApu = nesApu;
    mNesEnvelope = nesEnvelope;
    mChannel = channel;
    mNoteTable = NesApu::GetNoteTableForChannel(channel);
  }

  void ProcessEffects() {}
//  void Advance() {}
//  void PlayNote(Note newNote) {
//    note = newNote;
//  }
//  void UpdateEnvelope() {}
//  void WriteRegister() {}
  bool IsSeeking() { return false; }
//  int GetEnvelopeFrame() {}
//  void ClearNote() {}
  int MultiplyVolumes(int v0, int v1) {
    auto vol = (int)roundf((v0 / 15.0f) * (v1 / 15.0f) * 15.0f);
    if (vol == 0 && v0 != 0 && v1 != 0) return 1;
    return vol;
  }

  int GetPeriod() {
//    int noteVal = clamp(note.Value + envelopeValues[Envelope.Arpeggio], 0, noteTable.size() - 1);
//    int pitch = (note.FinePitch + envelopeValues[Envelope.Pitch]) << pitchShift;
//    int slide = slideShift < 0 ? (slidePitch >> -slideShift) : (slidePitch << slideShift); // Remove the fraction part.
//    return Utils.Clamp(noteTable[noteVal] + pitch + slide, 0, maximumPeriod);
//    printf("Note value: %d Period: %d\n", note.value, noteTable[note.value]);


    // TODO: don't share envelopes among channels
    int noteEnv = mNesEnvelope->GetValueAndAdvance();

    return mNoteTable[mBaseNote + noteEnv];
  }
  int GetVolume() {
    // return MultiplyVolumes(note.Volume, envelopeValues[Envelope.Volume]);
    return 15;
  }
  int GetDuty() {
    // return envelopeValues[Enveelope.DutyCycle];
    return 2;
  }
  // TODO rename something like Advance() or EndFrame()
  void UpdateAPU() {
//    noteTriggered = false;
  }

// TODO
  void Trigger(int baseNote) {
    mBaseNote = baseNote;
    mNesEnvelope->Trigger(); // TODO: foreach
    UpdateAPU();
  }

  void Release() {
    mNesEnvelope->Release(); // TODO: foreach
    UpdateAPU();
  }
//protected:
  shared_ptr<Simple_Apu> mNesApu;
  NesApu::Channel mChannel;
  //Note note = new Note(Note.NoteInvalid);
//  bool noteTriggered = false;
  bool pitchEnvelopeOverride = false;
  array<ushort, 97> mNoteTable;
//  Note note;
  int mBaseNote = 40;
  int maxPeriod = 2047;
  //  int envelopeIdx[] = new int[Envelope.Count];
  //  int envelopeValues[] = new int[Envelope.Count];
  shared_ptr<NesEnvelope> mNesEnvelope;

};

class NesChannelPulse : public NesChannel
{
public:
  int regOffset = 0;
  int prevPeriodHi = 1000;

  NesChannelPulse(shared_ptr<Simple_Apu> nesApu, NesApu::Channel type, shared_ptr<NesEnvelope> nesEnvelope) : NesChannel(nesApu, type, nesEnvelope)
  {
    // 0x4000 for Square1, 0x4004 for Square2
    regOffset = mChannel * 4;
  }

  void UpdateAPU() {
    int duty = GetDuty();
    int volume = 0;

    if (mNesEnvelope->GetState() != NesEnvelope::ENV_OFF) {
      int period = GetPeriod();
      volume = GetVolume();

      int periodHi = (period >> 8) & 0x07;
      int periodLo = (period >> 0) & 0xff;
      int deltaHi = periodHi - prevPeriodHi;

      if (deltaHi != 0) {
        // TODO: get smoothVibrato from some setting
        bool smoothVibrato = false;
        if (smoothVibrato && abs(deltaHi) == 1 && !IsSeeking()) {
          // Blaarg's smooth vibrato technique using the sweep
          // to avoid resetting the phase. Cool stuff.
          // http://forums.nesdev.com/viewtopic.php?t=231

          // reset frame counter in case it was about to clock
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0x40);
          // be sure low 8 bits of timer period are $FF ($00 when negative)
          mNesApu->write_register(NesApu::APU_PL1_LO + regOffset, deltaHi < 0 ? 0x00 : 0xff);
          // sweep enabled, shift = 7, set negative flag.
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + regOffset, deltaHi < 0 ? 0x8f : 0x87);
          // clock sweep immediately
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0xc0);
          // disable sweep
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + regOffset, 0x08);
        } else {
          mNesApu->write_register(NesApu::APU_PL1_HI + regOffset, periodHi);
        }

        prevPeriodHi = periodHi;
      }

      mNesApu->write_register(NesApu::APU_PL1_LO + regOffset, periodLo);
    }

    // duty is shifted to 2 most significant bits
    mNesApu->write_register(NesApu::APU_PL1_VOL + regOffset, (duty << 6) | 0x30 | volume);
    NesChannel::UpdateAPU();
  }
};

class NesChannelTriangle : public NesChannel
{
public:
  NesChannelTriangle(shared_ptr<Simple_Apu> nesApu, NesApu::Channel type, shared_ptr<NesEnvelope> nesEnvelope) : NesChannel(nesApu, type, nesEnvelope) {}

  void UpdateAPU() {
//            if (note.IsStop)
//            {
//                WriteRegister(NesApu.APU_TRI_LINEAR, 0x80);
//            }
//            else if (note.IsMusical)
//            {
//                var period = GetPeriod();
//
//                WriteRegister(NesApu.APU_TRI_LO, (period >> 0) & 0xff);
//                WriteRegister(NesApu.APU_TRI_HI, (period >> 8) & 0x07);
//                WriteRegister(NesApu.APU_TRI_LINEAR, 0x80 | envelopeValues[Envelope.Volume]);
//            }
//
//            base.UpdateAPU();

    if (mNesEnvelope->GetState() != NesEnvelope::ENV_OFF) {
      int period = GetPeriod();
      int periodLo = (period >> 0) & 0xff;
      int periodHi = (period >> 8) & 0x07;

      mNesApu->write_register(NesApu::APU_TRI_LO, periodLo);
      mNesApu->write_register(NesApu::APU_TRI_HI, periodHi);
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0xff);
    } else {
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0x80);
    }

    NesChannel::UpdateAPU();
  }
};



#endif /* NesChannelState_h */
