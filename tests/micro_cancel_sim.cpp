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
            vy += held ? 0.8 : -0.8;
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

    auto plan = planMicroCancel(waveGate(), [&](int releaseAt) {
        if (releaseAt >= tapTicks) return Verdict::Bad;
        auto recovery = recoveryAt(Model::WaveSpam, 14.0, 0.0, releaseAt,
                                   naturalPressAt, jitter, tapTicks, horizon);
        return recovery.robust() ? Verdict::Good : Verdict::Bad;
    });
    check(plan.releaseAfterTicks == 1, "wave should choose the earliest robust one-tick release");

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

    auto plan = planMicroCancel(shipGate(), [&](int releaseAt) {
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
    testLatchRejectsInvalidClockAndConsumesOnce();
    std::printf("Phase 2 micro-cancel checks: %d assertions, %d failures\n", assertions, failures);
    return failures ? 1 : 0;
}
