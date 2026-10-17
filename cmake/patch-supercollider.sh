#!/bin/sh
# Lets SuperCollider's source build as a subdirectory: its CMake files locate sources from
# CMAKE_SOURCE_DIR, which is then sapf-sc's root, so they use their own PROJECT_SOURCE_DIR; and
# sclang, the platform files and the editors are left out. Also keeps the UGen plugins loaded when
# an NRT render ends: World_NonRealTimeSynthesis unloads them, which crashes the next World and
# any realtime one still running. World_Cleanup unloads the plugins before it stops the audio and
# frees the synths, so the audio thread and the Unit destructors ran unmapped code; it now unloads
# them after both, and before the buffers DiskIO's disk thread writes. Drops the Green FFT's init
# banner, which it prints to stdout before main and so corrupts `stac --words` JSON. Renames the
# device drivers' factory to SC_NewDeviceAudioDriver, so src/server/HostAudioDriver.cpp can choose
# between it and a host-driven driver. Run in SC's source folder. Idempotent. Arguments are ignored.
set -e
find . \( -name 'CMakeLists.txt' -o -name '*.cmake' \) -exec sed -i.bak 's|\${CMAKE_SOURCE_DIR}|\${PROJECT_SOURCE_DIR}|g' {} +
sed -i.bak -e '/^add_subdirectory(lang)$/d' -e '/^add_subdirectory(platform)$/d' -e '/^add_subdirectory(editors)$/d' CMakeLists.txt
perl -0pi -e 's/(free\(packet\.mData\);\n\s*)World_Cleanup\(world, true\);/$1World_Cleanup(world, false);/' server/scsynth/SC_World.cpp
perl -0pi -e 's/(    if \(unload_plugins\)\n        deinitialize_library\(\);\n)\n(.*?\n)(    World_Free\(world, world->mTopGroup\);)/$2$1$3/s' server/scsynth/SC_World.cpp
sed -i.bak 's/^SC_AudioDriver\* SC_NewAudioDriver(/SC_AudioDriver* SC_NewDeviceAudioDriver(/' server/scsynth/SC_PortAudio.cpp server/scsynth/SC_CoreAudio.cpp
sed -i.bak '/SC FFT global init: cosTable initialised/d' common/SC_fftlib.cpp
find . -name '*.bak' -delete
