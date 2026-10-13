# LocalBuf proof of concept

Question: can a SynthDef on the SSP's `scsy` module record and manipulate samples without host-allocated buffers?

Answer on the host: yes, through `LocalBuf`. `scsy` itself is untested.

## The def

`poc/looper.py` builds `looper`. It records the first 10 s of input 1 once with `RecordBuf` into a `LocalBuf`. Then `BufRd` loops it to output 1 at `rate` (p1, -2..2). Output 2 is input 1.

- The buffer is `SampleRate.ir() * seconds` frames, so the def works at any rate.
- nanosynth inserts the `MaxLocalBufs` UGen that `LocalBuf` needs.

## Results

nanosynth 0.4.x NRT render, scsynth 3.14.1, macOS arm64. `tests/test_looper.py` checks the first three.

| Check | Result |
|-|-|
| Loop of 1 s at 48 kHz, 2.5 s render | sample-exact on every pass, the recording pass included |
| Loop of 0.5 s at 44.1 kHz | period is 22050 frames |
| Loop of 60 s, default 8192 KB pool | `LocalBuf_allocBuffer: alloc failed`; output silent |
| Largest loop in the default pool (bisection) | 2,092,299 frames: 43.6 s mono at 48 kHz, 8173 KB |

`scsy`'s `ScWorld` does not set `mRealTimeMemorySize`, so it gets the same 8192 KB pool. Each `scsy` instance runs its own World with its own pool. All `LocalBuf`s in one def share it with the other UGens.

## Limits of route 1

- About 43 s of mono audio per instance unless `ScWorld` raises the pool.
- Loading a new def discards the recording.
- No sample files: that needs `/b_allocRead` (route 2).

## On the SSP

```
make defs        # build/scsy/looper.scsyndef and looper.txt
```

Copy both files to `/media/BOOT/scsy`, load `looper` in `scsy`, and patch audio into input 1. Still to check on the device:

1. The loop starts after 10 s and repeats the first 10 s.
2. p1 changes speed and direction.
3. Two `scsy` instances each running `looper` stay within their own pools.
