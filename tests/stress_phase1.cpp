#include "BufferCore.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

using namespace ibcore;

namespace {
int failures = 0;
long long checks = 0;

void check(bool condition, char const* message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::printf("FAIL: %s\n", message);
    }
}

struct Rng {
    uint32_t state = 0x8d12e5a7u;
    uint32_t next() {
        state = state * 1664525u + 1013904223u;
        return state;
    }
    int range(int n) { return static_cast<int>(next() % static_cast<uint32_t>(n)); }
};

void testProfileBounds() {
    std::puts("[phase1] profile bounds");
    for (int value = -100; value <= 100; ++value) {
        int const normalized = microCancelMaxTicks(value);
        check(normalized >= 0 && normalized <= 3, "profile escaped [0,3]");
        int const next = nextMicroCancelProfile(value);
        check(next >= 0 && next <= 3, "profile cycle escaped [0,3]");
        check(next == (normalized + 1) % 4, "profile cycle did not normalize first");
    }
    check(microCancelMaxTicks(std::numeric_limits<int>::min()) == 0, "INT_MIN profile clamp");
    check(microCancelMaxTicks(std::numeric_limits<int>::max()) == 3, "INT_MAX profile clamp");
    check(nextMicroCancelProfile(std::numeric_limits<int>::min()) == 1, "INT_MIN profile cycle");
    check(nextMicroCancelProfile(std::numeric_limits<int>::max()) == 0, "INT_MAX profile cycle");
}

void testEligibilityTruthTable() {
    std::puts("[phase1] exhaustive micro-cancel gate truth table");
    // 12 independent boolean switches, every combination, plus invalid and
    // boundary profile values. The expected rule is written independently.
    constexpr unsigned combinations = 1u << 12;
    for (unsigned bits = 0; bits < combinations; ++bits) {
        for (int maxTicks = -1; maxTicks <= 4; ++maxTicks) {
            MicroCancelGate g;
            g.enabled          = (bits & (1u << 0)) != 0;
            g.press            = (bits & (1u << 1)) != 0;
            g.wave             = (bits & (1u << 2)) != 0;
            g.ship             = (bits & (1u << 3)) != 0;
            g.waveSpam         = (bits & (1u << 4)) != 0;
            g.straightFlight   = (bits & (1u << 5)) != 0;
            g.dual             = (bits & (1u << 6)) != 0;
            g.queueActive      = (bits & (1u << 7)) != 0;
            g.alreadyHolding   = (bits & (1u << 8)) != 0;
            g.baselineDied     = (bits & (1u << 9)) != 0;
            g.baselineTruncated= (bits & (1u << 10)) != 0;
            g.baselineBudget   = (bits & (1u << 11)) != 0;
            g.maxTicks = maxTicks;

            bool const expectedBase = g.enabled && g.press && !g.dual &&
                !g.queueActive && !g.alreadyHolding && g.baselineDied &&
                !g.baselineTruncated && !g.baselineBudget && maxTicks > 0;
            bool const expectedMode = (g.wave && !g.ship && g.waveSpam) ||
                (g.ship && !g.wave && g.straightFlight);
            check(microCancelEligible(g) == (expectedBase && expectedMode),
                  "gate truth-table mismatch");
        }
    }
}

void testSearchWindows() {
    std::puts("[phase1] randomized earliest-release windows");
    Rng rng;
    constexpr int trials = 50000;
    for (int i = 0; i < trials; ++i) {
        MicroCancelGate g;
        g.enabled = g.press = g.baselineDied = true;
        g.wave = true;
        g.waveSpam = true;
        g.maxTicks = 1 + rng.range(3);

        // 0 means no safe candidate; otherwise this is the first safe tick.
        int const safeAt = rng.range(g.maxTicks + 1);
        int const profileLimit = g.maxTicks;
        auto plan = planMicroCancel(g, [&](int tick) {
            return safeAt > 0 && tick == safeAt ? Verdict::Good : Verdict::Bad;
        });
        check(plan.releaseAfterTicks == safeAt, "search selected a non-earliest or unsafe tick");
        check(plan.evals == (safeAt > 0 ? safeAt : profileLimit),
              "search evaluated the wrong number of candidates");
    }

    for (Verdict uncertain : {Verdict::Partial, Verdict::Budget}) {
        for (int maxTicks = 1; maxTicks <= 3; ++maxTicks) {
            MicroCancelGate g;
            g.enabled = g.press = g.wave = g.waveSpam = g.baselineDied = true;
            g.maxTicks = maxTicks;
            int calls = 0;
            auto plan = planMicroCancel(g, [&](int) {
                ++calls;
                return uncertain;
            });
            check(!plan && plan.evals == 1 && calls == 1,
                  "unknown/budget candidate did not fail closed immediately");
        }
    }
}

void testInvalidClockFailsClosed() {
    std::puts("[phase1] invalid clock / epsilon handling");
    auto mustCancel = [](double now, double eps) {
        MicroCancelLatch latch;
        check(latch.arm(10.0), "could not arm invalid-clock fixture");
        bool const fired = latch.process(now, true, true, eps);
        check(!fired, "invalid clock/tolerance fired an automatic release");
        check(!latch.pending(), "invalid clock/tolerance left a stale pending release");
        check(!latch.released(), "invalid clock/tolerance set released latch");
        check(!latch.consumePhysicalRelease(), "invalid clock/tolerance swallowed a future physical release");
    };
    double const nan = std::numeric_limits<double>::quiet_NaN();
    double const inf = std::numeric_limits<double>::infinity();
    mustCancel(nan, 1e-6);
    mustCancel(inf, 1e-6);
    mustCancel(-inf, 1e-6);
    mustCancel(0.0, nan);
    mustCancel(0.0, inf);
    mustCancel(0.0, -1e-6);
}

void testRandomizedLatchLifecycle() {
    std::puts("[phase1] deterministic latch lifecycle fuzz (250,000 operations)");
    Rng rng;
    MicroCancelLatch latch;
    for (int i = 0; i < 250000; ++i) {
        int const op = rng.range(6);
        double const now = static_cast<double>(rng.range(100000)) / 100.0;
        if (op == 0) {
            bool const canArm = !latch.pending() && !latch.released();
            double const due = now + static_cast<double>(rng.range(300)) / 100.0;
            bool const result = latch.arm(due);
            check(result == canArm, "arm accepted while a one-shot was already active");
        } else if (op == 1 || op == 2) {
            bool const physical = rng.range(5) != 0;
            bool const game = rng.range(5) != 0;
            double const eps = static_cast<double>(rng.range(5)) * 1e-6;
            bool expected = false;
            if (latch.pending()) {
                if (!physical || !game) {
                    expected = false;
                } else {
                    expected = now + eps >= latch.dueAt();
                }
            }
            bool const fired = latch.process(now, physical, game, eps);
            check(fired == expected, "latch process disagreed with reference transition");
            if (fired) {
                check(latch.released() && !latch.pending(), "fired latch has invalid state");
                check(!latch.process(now + 100.0, true, true, eps), "one-shot release repeated");
            }
        } else if (op == 3) {
            bool const wasPending = latch.pending();
            check(latch.cancelPending() == wasPending, "cancelPending return/state mismatch");
        } else if (op == 4) {
            bool const wasReleased = latch.released();
            check(latch.consumePhysicalRelease() == wasReleased, "physical release consumed incorrectly");
            check(!latch.consumePhysicalRelease(), "physical release consumed twice");
        } else {
            latch.reset();
            check(!latch.pending() && !latch.released() && latch.dueAt() == -1.0,
                  "reset did not restore a clean state");
        }
        check(!(latch.pending() && latch.released()), "pending and released became true together");
        if (latch.pending()) {
            check(std::isfinite(latch.dueAt()), "pending latch has a non-finite deadline");
        } else {
            check(latch.dueAt() == -1.0, "inactive latch retained a deadline");
        }
    }
}

} // namespace

int main() {
    testProfileBounds();
    testEligibilityTruthTable();
    testSearchWindows();
    testInvalidClockFailsClosed();
    testRandomizedLatchLifecycle();
    std::printf("%lld phase-1 stress checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
