#include "OpenCVFrameProcessor.h"

namespace hawk_camera
{
    OpenCVFrameProcessor::OpenCVFrameProcessor()
    {
    }

    OpenCVFrameProcessor::~OpenCVFrameProcessor()
    {
    }

    void OpenCVFrameProcessor::ProcessFrame(uint32_t width, uint32_t height, uint8_t *address, uint32_t stride)
    {
        cv::Mat image(height, width, CV_8UC3, address, stride);

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_frames.emplace_back(image.clone());
        }
    }

    bool OpenCVFrameProcessor::HasFrame() const
    {
        return !m_frames.empty();
    }

    const cv::Mat OpenCVFrameProcessor::GetLatestFrame() const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_frames.empty())
        {
            return m_frames.front();
        }
        return cv::Mat();
    }

    void OpenCVFrameProcessor::RemoveLatestFrame()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_frames.empty())
        {
            m_frames.pop_front();
        }
    }

    void OpenCVFrameProcessor::ClearFrames()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_frames.clear();
    }

} // namespace hawk_camera