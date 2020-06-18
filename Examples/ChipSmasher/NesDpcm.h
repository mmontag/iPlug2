//
//  NesDpcm.h
//  APP
//
//  Created by Matt Montag on 6/9/20.
//

#ifndef NesDpcm_h
#define NesDpcm_h

#include "tmnt3hey.h"
#include "./resources/dmc/TinyToon1.h"
#include "./resources/dmc/TinyToon2.h"
#include "./resources/dmc/TinyToon3.h"
#include "./resources/dmc/TinyToon4.h"
#include "./resources/dmc/TinyToon5.h"
#include "./resources/dmc/TinyToon6.h"
#include <map>
#include <utility>

using namespace std;

struct NesDpcmSample {
  string name;
  vector<char> data;

  NesDpcmSample(unsigned char *bytes, int length, string name) {
    data.assign(bytes, bytes + length);

    for (auto it = next(data.rbegin()); it != data.rend(); ++it) {
      // Reduce pop at end of DMCs that have been padded with 0x00.
      // Pad with 0x55 to create silence instead.
      if (*it == 0) *it = 0x55; // 01010101
      else break;
    }

    this->name = std::move(name);
  }

  NesDpcmSample(vector<char> bytes, string name) {
    data = std::move(bytes);
    this->name = std::move(name);
  }

  int length() {
    return (int) data.size();
  }
};

struct NesDpcmPatch {
  int pitch = 15;
  bool loop = false;
  shared_ptr<NesDpcmSample> dpcmSample;

  NesDpcmPatch() = default;

  explicit NesDpcmPatch(shared_ptr<NesDpcmSample> s) : dpcmSample(std::move(s)) {};
  NesDpcmPatch(int p, int l, shared_ptr<NesDpcmSample> s) : pitch(p), loop(l), dpcmSample(std::move(s)) {};
};

class NesDpcm {
public:
  NesDpcm() {
    shared_ptr<NesDpcmSample> sample = make_shared<NesDpcmSample>(TMNT3__E300_dmc, TMNT3__E300_dmc_len, "TMNT Hey");
    mSamples.push_back(sample);
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C000_dmc, TinyToonA2__C000_dmc_len, "TinyTOon 1"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C1C0_dmc, TinyToonA2__C1C0_dmc_len, "TInyTOon 2"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C2C0_dmc, TinyToonA2__C2C0_dmc_len, "TInyTOon 3"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C340_dmc, TinyToonA2__C340_dmc_len, "TInyTOon 4"));
    mTestPatch = NesDpcmPatch(sample);

    mNoteMap.push_back(make_shared<NesDpcmPatch>(mSamples[0]));
    mNoteMap.push_back(make_shared<NesDpcmPatch>(mSamples[1]));
    mNoteMap.push_back(make_shared<NesDpcmPatch>(mSamples[2]));
    mNoteMap.push_back(make_shared<NesDpcmPatch>(mSamples[3]));
    mNoteMap.push_back(make_shared<NesDpcmPatch>(13, true, mSamples[4]));
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
    mNoteMap.push_back(make_shared<NesDpcmPatch>());
  }

  void AddSample(shared_ptr<NesDpcmSample> sample) {
    mSamples.push_back(sample);
  }

  char GetSampleForAddress(int offset) {
    int addr = 0;
    for (const auto &s : mSamples) {
      if (offset >= addr && offset < addr + s->data.size())
        return s->data[offset - addr];
      addr = (addr + s->data.size() + 63) & 0xffc0; // limit to 16384, align to 64 bit
    }
    return 0x55; // 01010101 (dpcm silence)
  }

  int GetAddressForSample(shared_ptr<NesDpcmSample> sample) {
    int addr = 0;
    for (const auto &s : mSamples) {
      if (s == sample) {
        return addr;
      }
      addr = (addr + s->data.size() + 63) & 0xffc0;
    }

    return addr;
  }

  shared_ptr<NesDpcmPatch> GetDpcmPatchForNote(int note) {
//    if (mNoteMap.has(note)) return make_optional(mNoteMap[note]);
//    return
    if (mNoteMap.empty()) return nullptr;
    return mNoteMap.at(note % mNoteMap.size());
  }

//protected:
  vector<shared_ptr<NesDpcmSample>> mSamples;
  vector<shared_ptr<NesDpcmPatch>> mNoteMap;
  NesDpcmPatch mTestPatch;
};

#endif /* NesDpcm_h */
