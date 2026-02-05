#!/bin/bash

mkdir -p build/Release
cmake -DCMAKE_BUILD_TYPE=Release -Wno-deprecated -DBUILD_EXAMPLES=0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -B build/Release -S .

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
  
  # Wait a moment for primary to initialize
  sleep 2
  
  echo "Starting replica instance..."
  ./Allolib-Kickstart &
  REPLICA_PID=$!
  echo "Replica instance started with PID: $REPLICA_PID"
  
  echo "Both instances running. Press Ctrl+C to stop."
  echo "Primary PID: $PRIMARY_PID, Replica PID: $REPLICA_PID"
  
  # Wait for user to stop
  trap "echo 'Stopping instances...'; kill $PRIMARY_PID $REPLICA_PID 2>/dev/null; exit" INT
  wait
fi
