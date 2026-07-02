/*
 * HBM V2 — Time Rewind System
 * ---------------------------------------------------------------
 * Hooks PlayLayer::update, stores a hidden checkpoint every N physics
 * ticks and, when the rewind keybind is held, replays checkpoints from
 * the ring buffer backwards to produce a smooth rewind effect.
 *
 * Default keybind: N (configurable via Geode's custom-keybinds).
 */

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/Event.hpp>

#include <geode.custom-keybinds/include/Keybinds.hpp>

#include "TimeRewindManager.hpp"

using namespace geode::prelude;
using namespace keybinds;

static constexpr const char* kRewindBind = "time-rewind"_spr;

$execute {
    // Register the Time Rewind keybind, default = N, category "HBM V2".
    BindManager::get()->registerBindable({
        kRewindBind,
        "Time Rewind",
        "Hold to rewind gameplay from the snapshot buffer.",
        { Keybind::create(KEY_N, Modifier::None) },
        "HBM V2"
    });
}

class $modify(HBMPlayLayer, PlayLayer) {
    struct Fields {
        EventListener<InvokeBindFilter> rewindListener;
        bool  initialized     = false;
        bool  rewindHeld      = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto& mgr = TimeRewindManager::get();
        mgr.configureFromSettings();
        mgr.clear();
        mgr.tickCounter       = 0;
        mgr.isRewinding       = false;
        mgr.rewindAccumulator = 0.f;

        m_fields->rewindListener.bind([this](InvokeBindEvent* event) {
            // Time rewind only makes sense in Normal Mode. In Practice Mode we
            // yield to the game's built-in checkpoint UI instead.
            if (m_isPracticeMode) return ListenerResult::Propagate;
            m_fields->rewindHeld              = event->isDown();
            TimeRewindManager::get().isRewinding = event->isDown();
            return ListenerResult::Propagate;
        });
        m_fields->rewindListener.setFilter(InvokeBindFilter(this, kRewindBind));
        m_fields->initialized = true;
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto& mgr = TimeRewindManager::get();
        mgr.clear();
        mgr.tickCounter       = 0;
        mgr.isRewinding       = false;
        mgr.rewindAccumulator = 0.f;
        if (m_fields) m_fields->rewindHeld = false;
    }

    void onQuit() {
        TimeRewindManager::get().clear();
        PlayLayer::onQuit();
    }

    void update(float dt) {
        auto& mgr = TimeRewindManager::get();

        // Bail-outs where the rewind system must be a no-op.
        const bool systemDisabled =
            !m_fields->initialized ||
            m_isPracticeMode ||
            m_hasCompletedLevel ||
            m_isPaused;

        if (systemDisabled) {
            PlayLayer::update(dt);
            return;
        }

        // ---- Rewind phase ---------------------------------------------------
        if (mgr.isRewinding && mgr.hasSnapshots()) {
            mgr.rewindAccumulator += static_cast<float>(mgr.getRewindSpeed());
            int toConsume = static_cast<int>(mgr.rewindAccumulator);
            mgr.rewindAccumulator -= static_cast<float>(toConsume);

            // Fast-forward through however many snapshots the speed setting asks
            // for, but stop early when we reach the beginning of the buffer.
            while (toConsume-- > 0 && mgr.hasSnapshots()) {
                auto snap = mgr.popLatest();
                if (snap.checkpoint) {
                    // Native restore — handles triggers, saw rotations,
                    // wave trail, particles, camera, etc.
                    this->loadFromCheckpoint(snap.checkpoint);
                }
            }
            // Freeze normal simulation while the buffer is being rewound.
            return;
        }

        // ---- Normal simulation ---------------------------------------------
        PlayLayer::update(dt);

        // If the player is dead we still let update() run (respawn logic etc.)
        // but we don't want to poison the buffer with post-death states.
        if (m_isDead) return;

        // ---- Snapshot capture ----------------------------------------------
        // Only every N ticks — that's the whole optimisation. Native
        // createCheckpoint is O(objects on screen), so keeping the cadence
        // sparse is what avoids micro-freezes.
        mgr.tickCounter++;
        if (mgr.tickCounter >= mgr.getSnapshotInterval()) {
            mgr.tickCounter = 0;
            if (auto* cp = this->createCheckpoint()) {
                // createCheckpoint() also pushes into PlayLayer::m_checkpointArray.
                // Remove it from there so the practice-mode UI doesn't render our
                // hidden checkpoints as green diamonds.
                if (m_checkpointArray && m_checkpointArray->count() > 0) {
                    m_checkpointArray->removeLastObject();
                }
                mgr.pushSnapshot(cp, m_gameState.m_levelTime);
            }
        }
    }
};
