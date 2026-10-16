"""SCgf parity with nanosynth for every generated UGen word that nanosynth also has and that sends
each argument as one input. Each argument gets a distinct value, so a swapped slot changes the bytes.
nanosynth's rate methods list their parameters in wire order, so values map across by position."""

import inspect
import json
import sys

import pytest

from conftest import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
import gen_ugens  # noqa: E402

import nanosynth.ugens as ns_ugens  # noqa: E402
from nanosynth import SynthDefBuilder  # noqa: E402
from nanosynth.ugens import K2A, Out  # noqa: E402

METHOD = {"rateAR": "ar", "rateKR": "kr", "rateIR": "ir"}


def rows():
    spec = json.loads(gen_ugens.SPEC.read_text())
    wires = json.loads(gen_ugens.WIRE.read_text())
    overlay = json.loads(gen_ugens.OVERLAY.read_text())
    for group, classes in gen_ugens.groups(spec, wires, overlay).items():
        for cls in classes:
            yield group, gen_ugens.ugen(cls, spec, wires, overlay)


def cases():
    for group, u in rows():
        reason = None
        cls = getattr(ns_ugens, u["cls"], None)
        rate = u["rates"][0]
        if cls is None:
            reason = "not in nanosynth"
        elif rate not in METHOD:
            reason = "demand rate: needs a Demand around it"
        elif any(k != "wireArg" for k, _, _ in u["wire"]) or u["outputs_kind"] != "outputsFixed" or u["audio"]:
            reason = "splices, variable outputs or audio-rate inputs: not swept"
        elif u["outputs"] == 0:
            reason = "no outputs: a def needs a channel"
        elif len(inspect.signature(getattr(cls, METHOD[rate])).parameters) != len(u["wire"]):
            reason = "nanosynth's parameters differ in number"
        marks = [pytest.mark.skip(reason=reason)] if reason else []
        yield pytest.param(group, u, id=u["word"], marks=marks)


@pytest.mark.parametrize("group,u", list(cases()))
def test_parity(compile_sapf, group, u):
    values = [1.25 + i for i in range(len(u["args"]))]
    form = " ".join(f":{name} {v}" for (name, _, _), v in zip(u["args"], values))
    word = f"sc.{u['word']}"
    sapf = compile_sapf(f"{{{form}}} {word}")

    method = METHOD[u["rates"][0]]
    params = list(inspect.signature(getattr(getattr(ns_ugens, u["cls"]), method)).parameters)
    kwargs = {p: values[arg] for p, (_, arg, _) in zip(params, u["wire"])}
    with SynthDefBuilder() as b:
        sig = getattr(getattr(ns_ugens, u["cls"]), method)(**kwargs)
        Out.ar(bus=b.add_parameter(name="out", value=0), source=sig if method == "ar" else K2A.ar(source=sig))
    assert sapf == b.build(name="t").compile()
