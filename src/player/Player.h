/**
 * dubby-dub
 *
 * Copyright (C) 2020, Loïc Le Page
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "Connector.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

class IPlayerListener;

class Player final
{
  public:
    enum class ErrorCode : int
    {
        undefined,
        cannotCreateConnector,
        cannotInitializePipeline
    };
    static const GQuark errorDomain;

    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = default;
    Player& operator=(Player&&) = default;

    void addPlayerListener(const std::shared_ptr<IPlayerListener>& listener) noexcept;
    void removePlayerListener(const std::shared_ptr<IPlayerListener>& listener) noexcept;

    const Glib::RefPtr<Gst::Pipeline>& getPipeline() const noexcept
    {
        return m_pipeline;
    }

    void forEachConnector(const std::function<void(Connector&)>& cb);

    enum class State
    {
        undefined,
        prerolled,
        playing,
        stopped
    };
    bool hasStableState(State state) const noexcept;

    void play(const Glib::ustring& uri);
    void stop() noexcept;

  private:
    Glib::RefPtr<Gst::Pipeline> m_pipeline;
    unsigned int m_busWatchId;

    Glib::RefPtr<Gst::UriDecodeBin> m_uriDecodeBin;
    std::atomic_int m_prerollingPads;
    std::atomic_bool m_prerollDone;

    std::vector<Connector> m_connectors;
    std::mutex m_connectorsWriteLock;

    std::vector<std::weak_ptr<IPlayerListener>> m_listeners;

    State m_currentState;
    State m_pendingState;
    bool m_interrupted;

    bool onBusMessage(const Glib::RefPtr<Gst::Bus>& bus, const Glib::RefPtr<Gst::Message>& message) noexcept;
    void onPadAdded(const Glib::RefPtr<Gst::Pad>& pad) noexcept;
    void onPadPrerolled() noexcept;

    void triggerPlayerPrerolled();
    void triggerPlayerPlaying() noexcept;
    void triggerPlayerStopped() noexcept;
    void triggerPipelineIssue(bool isFatalError, const Glib::Error& error, const std::string& debugMessage) noexcept;
};
