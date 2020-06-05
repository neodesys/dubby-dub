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

#include "Encoder.h"
#include "ITranscoderListener.h"
#include <atomic>
#include <mutex>
#include <vector>

class Transcoder final
{
  public:
    enum class ErrorCode : int
    {
        undefined,
        cannotInitializeOutputTee,
        cannotRequestTeePad
    };
    static const GQuark errorDomain;

    Transcoder();
    ~Transcoder();

    Transcoder(const Transcoder&) = delete;
    Transcoder& operator=(const Transcoder&) = delete;
    Transcoder(Transcoder&&) = default;
    Transcoder& operator=(Transcoder&&) = default;

    void addEncoder(const std::shared_ptr<Encoder>& encoder);
    void clearEncoders();

    void addTranscoderListener(const std::shared_ptr<ITranscoderListener>& listener) noexcept;
    void removeTranscoderListener(const std::shared_ptr<ITranscoderListener>& listener) noexcept;

    void startTranscoding(const Glib::ustring& uri);
    void stopTranscoding() noexcept;

  private:
    Glib::RefPtr<Gst::Pipeline> m_pipeline;
    unsigned int m_busWatchId;
    Glib::RefPtr<Gst::UriDecodeBin> m_uriDecodeBin;
    std::atomic_int m_prerollingPads;
    std::atomic_bool m_prerollDone;

    struct Connector
    {
        Glib::RefPtr<Gst::Tee> outputTee;
        Glib::RefPtr<Gst::Pad> decoderPad;
        unsigned long blockingProbeId;
        GstStreamType type;
    };
    std::vector<Connector> m_connectors;
    std::mutex m_connectorsWriteLock;

    std::vector<std::shared_ptr<Encoder>> m_encoders;
    std::vector<std::weak_ptr<ITranscoderListener>> m_listeners;

    enum class State
    {
        undefined,
        prerolled,
        transcoding,
        stopped
    };
    State m_currentState;
    State m_pendingState;
    bool m_interrupted;

    void onPadAdded(const Glib::RefPtr<Gst::Pad>& pad) noexcept;
    void onPadPrerolled() noexcept;
    void onConfigurePipeline() noexcept;

    bool hasStableState(State state) const noexcept;
    bool onBusMessage(const Glib::RefPtr<Gst::Bus>& bus, const Glib::RefPtr<Gst::Message>& message) noexcept;

    void triggerTranscodingStarted() noexcept;
    void triggerTranscodingStopped() noexcept;
    void triggerTranscodingIssue(bool isFatalError, const Glib::Error& error) noexcept;

    bool prepareEncoders() noexcept;
    bool connectEncoders() noexcept;
    void disconnectEncoders() noexcept;
};
