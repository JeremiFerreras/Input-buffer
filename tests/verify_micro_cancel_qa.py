#!/usr/bin/env python3
"""Validate Micro-cancel real-game replay QA source/config invariants."""
from __future__ import annotations
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
SAMPLE = (ROOT / "tests" / "micro_cancel_qa.example.txt").read_text(encoding="utf-8")


def require(needle: str, message: str) -> None:
    if needle not in SOURCE:
        raise AssertionError(message)


def parse_sample():
    groups = defaultdict(list)
    current = None
    for line_no, raw in enumerate(SAMPLE.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if parts[0] == "job" and len(parts) == 5:
            current = {"name": parts[1], "group": parts[2],
                       "profile": int(parts[3]), "max_steps": int(parts[4]), "events": []}
            groups[current["group"]].append(current)
        elif parts[0] == "inputs" and current is not None:
            for token in parts[1:]:
                step, state = token.split(":", 1)
                current["events"].append((int(step), int(state)))
        else:
            raise AssertionError(f"invalid sample directive at line {line_no}")

    if not groups:
        raise AssertionError("sample contains no jobs")
    for group, jobs in groups.items():
        if len({j["max_steps"] for j in jobs}) != 1:
            raise AssertionError(f"{group}: unequal max_steps")
        tapes, profiles = set(), set()
        for job in jobs:
            events = job["events"]
            if len(events) < 4 or events[0][1] != 1 or events[-1][1] != 0:
                raise AssertionError(f"{job['name']}: missing press/release/manual follow-up")
            previous, expected = -1, 1
            for step, state in events:
                if step <= previous or state != expected or step >= job["max_steps"]:
                    raise AssertionError(f"{job['name']}: invalid event tape")
                previous, expected = step, 1 - expected
            tapes.add(tuple(events))
            profiles.add(job["profile"])
        if len(tapes) != 1:
            raise AssertionError(f"{group}: profiles have different input tapes")
        if 0 not in profiles or len(profiles) != len(jobs) or len(profiles) < 2:
            raise AssertionError(f"{group}: requires unique OFF and active profiles")
    return groups


def main():
    checks = (
        ("std::map<std::string, int> groupMaxSteps;", "parser must track group horizons"),
        ("must use the same max_steps", "parser must reject unequal horizons"),
        ("job.inputs.size() < 4", "parser must require later manual follow-up"),
        ("std::ios::out | std::ios::trunc", "results must be batch-local"),
        ("probe.csv", "candidate probe output must exist"),
        ("writeProbeRows()", "candidate branches must be exported after engine steps"),
        ("microCancelQaGateFlags", "eligibility gate flags must be recorded"),
        ("candidate.clearAll", "candidate full-path clearance must be captured"),
        ("candidate.killType", "candidate killer type must be captured"),
        ("qaProbe->baseline.killType = r0.killType", "original fatal baseline details must be captured"),
        ("baseline_clearance", "probe CSV must include original baseline clearance"),
        ("microCancelQaProbeReasonName", "probe rejection reasons must be labelled"),
        ("followupVerified(test)", "save classification must verify follow-up"),
        ("SURVIVED_UNVERIFIED_FOLLOWUP", "unverified survival must not count as a save"),
        ("validReplayStatus(base->status)", "invalid baseline status must be rejected"),
        ("q.awaitingManualPress", "trace must include follow-up state"),
        ("followupState(q.awaitingManualPress", "trace must label follow-up state"),
        ("followupState(q.awaitingManualPress, q.expectedPressAt, q.actualPressAt, true)", "final output must label missing follow-up"),
        ("level->m_levelType = s_levelType", "source level type must be preserved"),
        ("MicroCancelReplayHarness::beforeStep(this)", "tape must be injected before game step"),
        ("MicroCancelReplayHarness::afterStep(pl, dt, isHalfTick, isLastTick)", "state must be sampled after game step"),
    )
    for needle, message in checks:
        require(needle, message)
    groups = parse_sample()
    print(f"Micro-cancel replay QA: {len(groups)} groups / {sum(map(len, groups.values()))} jobs - PASS")


if __name__ == "__main__":
    main()
