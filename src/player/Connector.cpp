#include "Connector.h"
#include "../exceptions.h"

Connector::Connector(const Glib::RefPtr<Gst::Pad>& srcPad, const Gst::Pad::SlotProbe& blockingProbeSlot)
    : m_blockingProbeId(0), m_streamType(GST_STREAM_TYPE_UNKNOWN)
{
    auto name = srcPad->get_current_caps()->get_structure(0).get_name();
    if (name.compare("video/x-raw") == 0)
    {
        m_streamType = GST_STREAM_TYPE_VIDEO;
    }
    else if (name.compare("audio/x-raw") == 0)
    {
        m_streamType = GST_STREAM_TYPE_AUDIO;
    }

    m_outputTee = Gst::Tee::create();
    m_outputTee->property_allow_not_linked() = true;

    auto parent = Glib::RefPtr<Gst::Bin>::cast_static(srcPad->get_parent_element()->get_parent());
    parent->add(m_outputTee);

    auto teeSinkPad = m_outputTee->get_static_pad("sink");
    if (srcPad->link(teeSinkPad) != Gst::PAD_LINK_OK)
    {
        throw CannotLinkPadException();
    }

    if (!m_outputTee->sync_state_with_parent())
    {
        throw InvalidStateException();
    }

    m_blockingProbeId = srcPad->add_probe(Gst::PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, blockingProbeSlot);
    m_srcPad = srcPad;
}

Connector::~Connector()
{
    unblock();

    if (m_outputTee)
    {
        m_outputTee->set_state(Gst::STATE_NULL);

        auto parent = Glib::RefPtr<Gst::Bin>::cast_static(m_outputTee->get_parent());
        if (parent)
        {
            parent->remove(m_outputTee);
        }

        auto it = m_outputTee->iterate_src_pads();
        while (it.next() == Gst::ITERATOR_OK)
        {
            m_outputTee->release_request_pad(*it);
        }
    }
}

void Connector::connect(const Glib::RefPtr<Gst::Pad>& sinkPad)
{
    auto pad = m_outputTee->get_request_pad("src_%u");
    if (pad->link(sinkPad) != Gst::PAD_LINK_OK)
    {
        throw CannotLinkPadException();
    }
}

void Connector::unblock() noexcept
{
    if (m_srcPad)
    {
        m_srcPad->remove_probe(m_blockingProbeId);
        m_srcPad.reset();
        m_blockingProbeId = 0;
    }
}
