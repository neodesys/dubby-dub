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

#include "encoders/Encoder.h"
#include <glibmm/main.h>

class Transcoder final : public IPlayerListener, public ISerializable
{
  public:
    static constexpr const char* type = "transcoder";

    static std::shared_ptr<Transcoder> create(int argc, char** argv, bool forceSoftwareEncoding = false);
    ~Transcoder() final = default;

    Transcoder(const Transcoder&) = delete;
    Transcoder& operator=(const Transcoder&) = delete;
    Transcoder(Transcoder&&) = default;
    Transcoder& operator=(Transcoder&&) = default;

    void addEncoder(const std::shared_ptr<Encoder>& encoder);
    void clearEncoders();

    void transcode(const Glib::ustring& uri);
    void interruptTranscoding() noexcept;

    void onPlayerPrerolled(Player& player) final;
    void onPlayerPlaying(Player& player) noexcept final;
    void onPlayerStopped(Player& player, bool isInterrupted) noexcept final;
    void onPipelineIssue(Player& player, bool isFatalError, const Glib::Error& error,
                         const std::string& debugMessage) noexcept final;

    Json serialize() const final;
    void unserialize(const Json& in) final;

  private:
    Transcoder();
    Player m_player;
    Glib::RefPtr<Glib::MainLoop> m_mainLoop;
    std::vector<std::shared_ptr<Encoder>> m_encoders;
};
