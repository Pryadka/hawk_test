#ifndef __OPENCV_FRAME_PROCESSOR__
#define __OPENCV_FRAME_PROCESSOR__

#include "FrameProcessingInterface.h"
#include <list>
#include <thread>
#include <opencv2/opencv.hpp>

namespace hawk_camera
{

    class OpenCVFrameProcessor : public FrameProcessingInterface
    {
    public:
        OpenCVFrameProcessor();
        ~OpenCVFrameProcessor() override;

        void ProcessFrame(uint32_t height, uint32_t width, uint8_t *address, uint32_t stride) override;

        bool HasFrame() const;
        const cv::Mat GetLatestFrame() const;
        void RemoveLatestFrame();
        void ClearFrames();

    private:
        std::list<cv::Mat> m_frames;
        mutable std::mutex m_mtx;
    };

} // namespace hawk_camera

#endif