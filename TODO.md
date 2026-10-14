# TODO

## Critical

## High

## Medium

### Features

- [ ] `>sd`: write the SSP `scsy` sidecar from `ctl` names alongside `name.scsyndef`
  - Format, from `../ssp/plugins/scsynth/README.md`: `name.txt` beside `name.scsyndef`, lines `// @p1 cutoff 20 20000 Hz log cv 3` (slot, control name, min, max, unit, curve, CV input).
  - Without a sidecar, `scsy` maps `p1`..`p16` to the first 16 controls in def order, each ranged 0 to max(1, 2 x default). A sidecar is needed only for ranges, units, curves, CV or a different slot order.
  - Open: `ctl` takes only a default and a name, so min, max and curve need a source, e.g. a `ctl` variant taking a range.
  - Open: slot order. `>sd` sorts parameters by name; slots may need creation order.
  - Note: a label that names no control fails the load in `scsy`, and `freq`, `velocity` and `gate` are reserved for MIDI.

- [ ] `lpcombc`: its low pass sits inside the feedback loop, which needs single-sample feedback. Options: an approximation, or a plugin UGen.

## Low

- [ ] `ola` voices start on the 64-frame block holding their time. `OffsetOut` would place them on the frame but crashed nanosynth's NRT renderer (`world_nrt_render`); find out why.
