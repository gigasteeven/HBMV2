#include "TimeRewindManager.hpp"

#include <Geode/binding/CheckpointObject.hpp>

TimeRewindManager& TimeRewindManager::get() {
    static TimeRewindManager instance;
    return instance;
}

void TimeRewindManager::configureFromSettings() {
    auto mod = Mod::get();
    m_rewindDuration   = mod->getSettingValue<double>("rewind-duration");
    m_snapshotInterval = static_cast<int>(mod->getSettingValue<int64_t>("snapshot-interval"));
    m_rewindSpeed      = mod->getSettingValue<double>("rewind-speed");

    // GD runs physics at 240Hz (4 physics steps per rendered frame at 60fps).
    // Buffer size = duration_seconds * ticksPerSecond / interval.
    constexpr double kTicksPerSecond = 240.0;
    double raw = (m_rewindDuration * kTicksPerSecond) / std::max(1, m_snapshotInterval);
    m_maxSnapshots = static_cast<size_t>(raw);
    if (m_maxSnapshots < 8) m_maxSnapshots = 8;

    log::debug("[HBMV2] Time rewind configured: duration={}s, interval={}t, speed={}x, cap={} snapshots",
        m_rewindDuration, m_snapshotInterval, m_rewindSpeed, m_maxSnapshots);
}

void TimeRewindManager::pushSnapshot(CheckpointObject* cp, float ts) {
    if (!cp) return;

    // Ring behaviour: drop the oldest snapshot when the buffer is full.
    // Ref<> handles retain/release so no manual memory bookkeeping is required.
    if (m_buffer.size() >= m_maxSnapshots) {
        m_buffer.pop_front();
    }
    m_buffer.push_back(Snapshot{ Ref<CheckpointObject>(cp), ts });
}

TimeRewindManager::Snapshot TimeRewindManager::popLatest() {
    if (m_buffer.empty()) return Snapshot{ nullptr, 0.f };
    auto snap = m_buffer.back();
    m_buffer.pop_back();
    return snap;
}
