//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// An scsynth audio driver run by a host's audio callback, as a plugin's: the World opens no device
// and World_HostRunBlock renders its output buses one block at a time. Built into libscsynth by
// cmake/EmbeddedScsynth.cmake; patch-supercollider.sh renames the device driver's factory to
// SC_NewDeviceAudioDriver, so SC_NewAudioDriver here picks one of the two.

#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_Prototypes.h"
#include "SC_WorldOptions.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

SC_AudioDriver* SC_NewDeviceAudioDriver(struct World* inWorld);
void sc_SetDenormalFlags();

namespace {

std::atomic<bool> gHostDriven{false};

class HostAudioDriver : public SC_AudioDriver {
public:
    explicit HostAudioDriver(World* inWorld): SC_AudioDriver(inWorld) {}

    // As SC_PortAudioDriver::PortAudioCallback, for one block.
    void runBlock(float* const* out, int numOut, int64 oscTime) {
        sc_SetDenormalFlags();
        World* world = mWorld;
        const int bufFrames = world->mBufLength;
        try {
            mFromEngine.Free();
            mToEngine.Perform();
            mOscPacketsToEngine.Perform();

            mOSCbuftime = oscTime;
            const int64 nextTime = oscTime + mOSCincrement;
            int64 schedTime;
            while ((schedTime = mScheduler.NextTime()) <= nextTime) {
                float diffTime = (float)(schedTime - oscTime) * mOSCtoSamples + 0.5;
                float diffTimeFloor = std::floor(diffTime);
                world->mSampleOffset = std::min(std::max((int)diffTimeFloor, 0), bufFrames - 1);
                world->mSubsampleOffset = diffTime - diffTimeFloor;
                SC_ScheduledEvent event = mScheduler.Remove();
                event.Perform();
            }
            world->mSampleOffset = 0;
            world->mSubsampleOffset = 0.0f;

            World_Run(world);

            const int32 bufCounter = world->mBufCounter;
            for (int k = 0; k < numOut; ++k) {
                if (k < (int)world->mNumOutputs && world->mAudioBusTouched[k] == bufCounter)
                    std::memcpy(out[k], world->mAudioBus + k * bufFrames, bufFrames * sizeof(float));
                else
                    std::memset(out[k], 0, bufFrames * sizeof(float));
            }
            world->mBufCounter++;
            mOSCbuftime = nextTime;
        } catch (std::exception& exc) {
            scprintf("HostAudioDriver: exception in real time: %s\n", exc.what());
        } catch (...) { scprintf("HostAudioDriver: unknown exception in real time\n"); }
        mAudioSync.Signal(); // wakes the NRT thread, which sends replies such as /synced
    }

protected:
    bool DriverSetup(int* outNumSamples, double* outSampleRate) override {
        if (!mPreferredSampleRate)
            return false;
        *outSampleRate = mPreferredSampleRate;
        *outNumSamples = mWorld->mBufLength;
        return true;
    }
    bool DriverStart() override { return true; }
    bool DriverStop() override { return true; }
};

} // namespace

SC_AudioDriver* SC_NewAudioDriver(struct World* inWorld) {
    if (gHostDriven)
        return new HostAudioDriver(inWorld);
    return SC_NewDeviceAudioDriver(inWorld);
}

// Realtime Worlds made after this call use the host driver, which needs mPreferredSampleRate.
SCSYNTH_DLLEXPORT_C void World_SetHostDriven(bool on) { gHostDriven = on; }

// Renders one block of mBufLength frames into out[0..numOut), the block starting at OSC time
// `oscTime`. Only for a World made host-driven.
SCSYNTH_DLLEXPORT_C void World_HostRunBlock(World* world, float* const* out, int numOut, int64 oscTime) {
    static_cast<HostAudioDriver*>(world->hw->mAudioDriver)->runBlock(out, numOut, oscTime);
}
