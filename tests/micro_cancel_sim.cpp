#include "BufferCore.hpp"

#include <algorithm>
#include <cstdio>

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
};

enum class Model { WaveSpam, StraightShip };

// Toy vertical corridor. Wave switches vertical direction on key-up; ship
// applies thrust while down and gravity while up. This is a deterministic
// GD-inspired fixture, not a substitute for the real Geometry Dash engine.
PathResult simulate(Model model, double startY, double startVy, int releaseAt, bool keepHeld) {
    constexpr double floorY = 0.0;
    constexpr double ceilingY = 22.0;
    PathResult out;
    double y = startY;
    double vy = startVy;
    bool held = true;
    out.presses = 1;
    for (int tick = 0; tick < 12; ++tick) {
        if (!keepHeld && releaseAt == tick) {
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

void testWaveSpamMicroSave() {
    auto baseline = simulate(Model::WaveSpam, 18.0, 0.0, -1, true);
    check(baseline.died, "wave spam baseline should die while held");
    auto plan = planMicroCancel(waveGate(), [&](int ticks) {
        auto result = simulate(Model::WaveSpam, 18.0, 0.0, ticks, false);
        return result.died ? Verdict::Bad : Verdict::Good;
    });
    check(plan.releaseAfterTicks == 1, "wave should find a 1-tick micro release");
    auto saved = simulate(Model::WaveSpam, 18.0, 0.0, plan.releaseAfterTicks, false);
    check(!saved.died && saved.presses == 1 && saved.releases == 1,
          "wave micro-cancel should save with exactly one press and one release");
    std::printf("wave spam: held dies at tick %d; one release at tick %d survives %d ticks\n",
                baseline.ticks, plan.releaseAfterTicks, saved.ticks);
}

void testStraightShipMicroSave() {
    auto baseline = simulate(Model::StraightShip, 18.0, 0.0, -1, true);
    check(baseline.died, "straight-ship baseline should die while held");
    auto plan = planMicroCancel(shipGate(), [&](int ticks) {
        auto result = simulate(Model::StraightShip, 18.0, 0.0, ticks, false);
        return result.died ? Verdict::Bad : Verdict::Good;
    });
    check(plan.releaseAfterTicks == 1, "straight ship should find a 1-tick release");
    auto saved = simulate(Model::StraightShip, 18.0, 0.0, plan.releaseAfterTicks, false);
    check(!saved.died && saved.presses == 1 && saved.releases == 1,
          "ship micro-cancel should save with exactly one press and one release");
    std::printf("straight ship: held dies at tick %d; one release at tick %d survives %d ticks\n",
                baseline.ticks, plan.releaseAfterTicks, saved.ticks);
}

void testNeverCancelsWithoutProof() {
    auto gate = waveGate();
    auto baseline = simulate(Model::WaveSpam, 21.0, 0.0, -1, true);
    check(baseline.died, "no-window baseline should die");
    auto plan = planMicroCancel(gate, [&](int ticks) {
        auto result = simulate(Model::WaveSpam, 21.0, 0.0, ticks, false);
        return result.died ? Verdict::Bad : Verdict::Good;
    });
    check(!plan, "no release should be approved when every candidate still dies");

    auto partial = planMicroCancel(gate, [](int) { return Verdict::Partial; });
    check(!partial && partial.evals == 1, "partial/unmodelled geometry must fail closed");

    gate.baselineTruncated = true;
    check(!microCancelEligible(gate), "truncated baseline must not arm");
    gate = waveGate();
    gate.baselineBudget = true;
    check(!microCancelEligible(gate), "budget exhaustion must not arm");
    gate = waveGate();
    gate.waveSpam = false;
    check(!microCancelEligible(gate), "ordinary non-spam wave input must not arm");
}
}

int main() {
    testWaveSpamMicroSave();
    testStraightShipMicroSave();
    testNeverCancelsWithoutProof();
    std::printf("%d GD-inspired simulation checks, %d failures\n", assertions, failures);
    return failures ? 1 : 0;
}
