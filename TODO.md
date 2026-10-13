# TODO

## Critical

## High

## Medium

### Features

- [ ] `>sd`: write the SSP `scsy` sidecar from `ctl` names alongside `name.scsyndef`
  - The POC (`git show 1d0abab:poc/looper.py`) wrote `looper.txt` holding `// @p1 rate -2 2`: slot, parameter name, min, max.
  - Open: the file name. The POC used `<name>.txt`, not `.scsy`; check what `scsy` loads.
  - Open: `ctl` takes only a default and a name, so min and max need a source, e.g. a `ctl` variant taking a range.
  - Open: slot order. `>sd` sorts parameters by name; slots may need creation order.

## Low
