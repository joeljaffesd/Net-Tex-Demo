#!/bin/bash

mkdir -p build/Release
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DRTAUDIO_API_JACK=OFF -DRTMIDI_API_JACK=OFF -Wno-deprecated -DBUILD_EXAMPLES=0 -B build/Release -S .

(
  # utilizing cmake's parallel build options
  # for cmake >= 3.12: -j <number of processor cores + 1>
  # for older cmake: -- -j5
  cmake --build build/Release --config Release -j 9
)

result=$?
if [ ${result} == 0 ]; then
  cd bin
  echo "Starting primary instance..."
  ./Allolib-Kickstart &
  PRIMARY_PID=$!
  echo "Primary instance started with PID: $PRIMARY_PID"
fi
