#include "BufferCore.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>

using namespace ibcore;

namespace {
int failures = 0;
int assertions = 0;

void check(bool condition, char const* message) {
    ++assertions;
    if (!condition) {
        ++failures;
        std::printf("FAIL: %s\n", message);
    }
}

struct PathResult {
    bool died = false;
    double minClearance = 1e9;
    int ticks = 0;
    int presses = 0;
    int releases = 0;
    int manualPresses = 0;
};

enum class Model { WaveSpam, StraightShip };

// Deterministic corridor fixture. It is intentionally only a policy test;
// runtime decisions use the hidden Geometry Dash PlayerObject simulator.
PathResult simulate(Model model, double startY, double startVy,
                    int automaticReleaseAt, int manualPressAt,
                    int manualReleaseAt, int horizon) {
    constexpr double floorY = 0.0;
    constexpr double ceilingY = 22.0;
    PathResult out;
    double y = startY;
    double vy = startVy;
    bool held = true;
    out.presses = 1;

    for (int tick = 0; tick < horizon; ++tick) {
        if (automaticReleaseAt == tick) {
            held = false;
            ++out.releases;
        }
        // These are hypothetical user events in the fixture only.
        if (manualPressAt == tick) {
            held = true;
            ++out.presses;
            ++out.manualPresses;
        }
        if (manualReleaseAt == tick) {
            held = false;
            ++out.releases;
        }

        if (model == Model::WaveSpam) {
            vy = held ? 2.0 : -2.0;
        } else {
            // Deliberately gentle ship acceleration in this corridor fixture:
            // it creates a held-path ceiling death while allowing a short
            // release followed by the user's ordinary tap to recover.
            vy += held ? 0.4 : -0.4;
            vy = std::clamp(vy, -3.0, 3.0);
        }
        y += vy;
        out.ticks = tick + 1;
        out.minClearance = std::min(out.minClearance, std::min(y - floorY, ceilingY - y));
        if (y <= floorY || y >= ceilingY) {
            out.died = true;
            break;
        }
    }
    return out;
}

MicroCancelGate waveGate(int maxTicks = 3) {
    MicroCancelGate gate;
    gate.enabled = true;
    gate.press = true;
    gate.wave = true;
    gate.waveSpam = true;
    gate.baselineDied = true;
    gate.maxTicks = maxTicks;
    return gate;
}

MicroCancelGate shipGate(int maxTicks = 3) {
    MicroCancelGate gate;
    gate.enabled = true;
    gate.press = true;
    gate.ship = true;
    gate.straightFlight = true;
    gate.baselineDied = true;
    gate.maxTicks = maxTicks;
    return gate;
}

MicroCancelRecoveryWindow recoveryAt(Model model, double startY, double startVy,
                                     int releaseAt, int naturalPressAt,
                                     int jitter, int tapTicks, int horizon) {
    MicroCancelRecoveryWindow result;
    result.earlySafe = !simulate(model, startY, startVy, releaseAt,
                                 naturalPressAt - jitter,
                                 naturalPressAt - jitter + tapTicks, horizon).died;
    result.nominalSafe = !simulate(model, startY, startVy, releaseAt,
                                   naturalPressAt,
                                   naturalPressAt + tapTicks, horizon).died;
    result.lateSafe = !simulate(model, startY, startVy, releaseAt,
                                naturalPressAt + jitter,
                                naturalPressAt + jitter + tapTicks, horizon).died;
    return result;
}

void testRecoveryWindowRequiresNaturalAndNeighbour() {
    MicroCancelRecoveryWindow recovery;
    recovery.nominalSafe = true;
    check(!recovery.robust(), "a single nominal frame-perfect recovery must be rejected");
    recovery.earlySafe = true;
    check(recovery.robust(), "nominal plus an early neighbouring timing is robust");
    recovery.earlySafe = false;
    recovery.lateSafe = true;
    check(recovery.robust(), "nominal plus a late neighbouring timing is robust");
}

void testWaveSpamMicroSave() {
    constexpr int tapTicks = 4;
    constexpr int naturalPressAt = 6;
    constexpr int jitter = 1;
    constexpr int horizon = naturalPressAt + jitter + tapTicks + 4;

    auto baseline = simulate(Model::WaveSpam, 14.0, 0.0, -1, -1, -1, horizon);
    check(baseline.died, "wave held baseline should die while rising");

    auto plan = planMicroCancelMinimal(waveGate(), [&](int releaseAt) {
        if (releaseAt >= tapTicks) return Verdict::Bad;
        auto recovery = recoveryAt(Model::WaveSpam, 14.0, 0.0, releaseAt,
                                   naturalPressAt, jitter, tapTicks, horizon);
        return recovery.robust() ? Verdict::Good : Verdict::Bad;
    });
    check(plan.releaseAfterTicks == 2, "wave should choose the latest robust release without losing recovery margin");

    auto saved = simulate(Model::WaveSpam, 14.0, 0.0, plan.releaseAfterTicks,
                          naturalPressAt, naturalPressAt + tapTicks, horizon);
    auto releaseOnly = simulate(Model::WaveSpam, 14.0, 0.0, plan.releaseAfterTicks,
                                -1, -1, horizon);
    check(!saved.died && saved.releases == 2 &&
          saved.manualPresses == 1 && saved.presses == 2,
          "wave recovery uses one automatic release and only the user's next tap");
    check(releaseOnly.died, "release-only does not keep playing or press for the user");
}

void testStraightShipMicroSave() {
    constexpr int tapTicks = 4;
    constexpr int naturalPressAt = 6;
    constexpr int jitter = 1;
    constexpr int horizon = naturalPressAt + jitter + tapTicks + 4;

    auto baseline = simulate(Model::StraightShip, 20.5, 0.0, -1, -1, -1, horizon);
    check(baseline.died, "straight-ship held baseline should die");

    auto plan = planMicroCancelMinimal(shipGate(), [&](int releaseAt) {
        if (releaseAt >= tapTicks) return Verdict::Bad;
        auto recovery = recoveryAt(Model::StraightShip, 20.5, 0.0, releaseAt,
                                   naturalPressAt, jitter, tapTicks, horizon);
        return recovery.robust() ? Verdict::Good : Verdict::Bad;
    });
    check(plan.releaseAfterTicks == 1, "straight ship should find a one-tick release with robust recovery");

    auto saved = simulate(Model::StraightShip, 20.5, 0.0, plan.releaseAfterTicks,
                          naturalPressAt, naturalPressAt + tapTicks, horizon);
    auto releaseOnly = simulate(Model::StraightShip, 20.5, 0.0, plan.releaseAfterTicks,
                                -1, -1, horizon);
    check(!saved.died && saved.releases == 2 &&
          saved.manualPresses == 1 && saved.presses == 2,
          "ship recovery uses one automatic release and only the user's next tap");
    check(releaseOnly.died, "ship release-only path must not pretend to guarantee survival");
}

void testNeverCancelsWithoutProof() {
    auto gate = waveGate();
    auto plan = planMicroCancel(gate, [](int) { return Verdict::Bad; });
    check(!plan, "no release should be approved when all candidates are bad");

    auto partial = planMicroCancel(gate, [](int) { return Verdict::Partial; });
    check(!partial && partial.evals == 1, "partial/unmodelled geometry must fail closed");

    auto budget = planMicroCancel(gate, [](int) { return Verdict::Budget; });
    check(!budget && budget.evals == 1, "budget exhaustion must fail closed");

    gate.baselineTruncated = true;
    check(!microCancelEligible(gate), "truncated baseline must not arm");
    gate = waveGate();
    gate.baselineBudget = true;
    check(!microCancelEligible(gate), "budget exhaustion must not arm");
    gate = waveGate();
    gate.waveSpam = false;
    check(!microCancelEligible(gate), "ordinary non-spam wave input must not arm");
}

void testEligibilityTruthTable() {
    for (unsigned mask = 0; mask < (1u << 11); ++mask) {
        MicroCancelGate g;
        g.enabled = (mask & (1u << 0)) != 0;
        g.press = (mask & (1u << 1)) != 0;
        g.wave = (mask & (1u << 2)) != 0;
        g.ship = (mask & (1u << 3)) != 0;
        g.waveSpam = (mask & (1u << 4)) != 0;
        g.straightFlight = (mask & (1u << 5)) != 0;
        g.dual = (mask & (1u << 6)) != 0;
        g.queueActive = (mask & (1u << 7)) != 0;
        g.alreadyHolding = (mask & (1u << 8)) != 0;
        g.baselineDied = (mask & (1u << 9)) != 0;
        g.baselineTruncated = (mask & (1u << 10)) != 0;
        g.baselineBudget = false;
        g.maxTicks = 3;
        bool const expected = g.enabled && g.press && !g.dual && !g.queueActive &&
            !g.alreadyHolding && g.baselineDied && !g.baselineTruncated &&
            ((g.wave && !g.ship && g.waveSpam) || (g.ship && !g.wave && g.straightFlight));
        check(microCancelEligible(g) == expected,
              "eligibility gate matches Wave/straight-Ship truth table");
    }
}

void testLatchDeterministicLifecycleFuzz() {
    // Fixed-seed xorshift; every run is deterministic and reproducible.
    uint32_t seed = 0x91E10DA5u;
    auto next = [&]() -> uint32_t {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed;
    };

    MicroCancelLatch latch;
    for (int i = 0; i < 5000; ++i) {
        latch.reset();
        double const due = 0.01 + static_cast<double>(next() % 100000u) / 10000.0;
        check(latch.arm(due), "fuzz cycle arms a finite deadline");
        check(!latch.process(due * 0.5, true, true), "fuzz cycle cannot release early");
        check(latch.pending(), "early poll keeps latch pending");
        check(latch.process(due, true, true), "fuzz cycle releases at exact deadline");
        check(!latch.pending() && latch.released(), "release enters one-shot released state");
        check(!latch.process(due + 1.0, true, true), "released latch cannot emit another release");
        check(latch.consumePhysicalRelease(), "one physical key-up is consumed");
        check(!latch.consumePhysicalRelease(), "second physical key-up is not swallowed");
        check(!latch.pending() && !latch.released(), "successful cycle returns to idle");
    }

    for (int i = 0; i < 2000; ++i) {
        latch.reset();
        double const due = 0.1 + static_cast<double>(next() % 10000u) / 1000.0;
        check(latch.arm(due), "cancel fuzz cycle arms");
        check(!latch.process(due * 0.5, false, true),
              "physical release before deadline cancels the automatic release");
        check(!latch.pending() && !latch.released(),
              "early physical release leaves no stale latch");
        check(!latch.consumePhysicalRelease(),
              "cancelled event cannot swallow unrelated future key-up");
    }
}

void testLatchRejectsInvalidClockAndConsumesOnce() {
    MicroCancelLatch latch;
    check(latch.arm(2.0), "valid release time arms");
    check(!latch.process(std::numeric_limits<double>::quiet_NaN(), true, true),
          "NaN clock must not release");
    check(!latch.pending(), "invalid clock cancels pending release");
    check(!latch.arm(std::numeric_limits<double>::infinity()),
          "infinite release time cannot arm");

    check(latch.arm(2.0), "valid release can arm after reset");
    check(latch.process(2.0, true, true), "due release fires once");
    check(latch.released(), "one matching physical key-up is pending");
    check(latch.consumePhysicalRelease(), "matching physical key-up is swallowed");
    check(!latch.consumePhysicalRelease(), "physical key-up cannot be swallowed twice");
}

} // namespace

int main() {
    testRecoveryWindowRequiresNaturalAndNeighbour();
    testWaveSpamMicroSave();
    testStraightShipMicroSave();
    testNeverCancelsWithoutProof();
    testEligibilityTruthTable();
    testLatchDeterministicLifecycleFuzz();
    testLatchRejectsInvalidClockAndConsumesOnce();
    std::printf("Phase 2 micro-cancel checks: %d assertions, %d failures\n", assertions, failures);
    return failures ? 1 : 0;
}
