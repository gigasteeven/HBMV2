#pragma once

#include <Geode/Geode.hpp>
#include <deque>

using namespace geode::prelude;

class CheckpointObject;

/**
 * Ring buffer of hidden checkpoints used by the Time Rewind system.
 *
 * Design notes:
 *  - We rely on the game's native checkpoint machinery (PlayLayer::createCheckpoint /
 *    loadFromCheckpoint) so that triggers (Move, Rotate, Alpha, etc.), object
 *    positions, saw rotations, particle emitters and the wave trail all rewind
 *    consistently — no hand-rolled physics needed.
 *  - We keep the CheckpointObject pointers in a std::deque and reuse the front
 *    slot when the buffer overflows, so the write path is O(1) and does not
 *    allocate on the hot path.
 *  - Ref<CheckpointObject> retains cocos ownership so the object survives even
 *    after PlayLayer prunes its own m_checkpointArray.
 */
class TimeRewindManager {
public:
    struct Snapshot {
        Ref<CheckpointObject> checkpoint;
        float timestamp = 0.f;
    };

    static TimeRewindManager& get();

    void configureFromSettings();

    void pushSnapshot(CheckpointObject* cp, float ts);
    Snapshot popLatest();

    bool hasSnapshots() const { return !m_buffer.empty(); }
    size_t size() const { return m_buffer.size(); }
    void clear() { m_buffer.clear(); }

    int    getSnapshotInterval() const { return m_snapshotInterval; }
    double getRewindSpeed()      const { return m_rewindSpeed;      }
    size_t getMaxSnapshots()     const { return m_maxSnapshots;     }

    // Live runtime flags — accessed only from the main thread (PlayLayer::update).
    bool  isRewinding       = false;
    int   tickCounter       = 0;
    float rewindAccumulator = 0.f;

private:
    TimeRewindManager() = default;

    std::deque<Snapshot> m_buffer;
    size_t m_maxSnapshots     = 300;
    double m_rewindDuration   = 5.0;
    int    m_snapshotInterval = 4;
    double m_rewindSpeed      = 2.0;
};
