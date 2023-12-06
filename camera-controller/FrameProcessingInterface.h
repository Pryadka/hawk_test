#ifndef __FRAME_PROCESSING_INTERFACE__
#define __FRAME_PROCESSING_INTERFACE__

#include <cstdint>

namespace hawk_camera
{

    class FrameProcessingInterface
    {
    public:
        FrameProcessingInterface() = default;
        virtual ~FrameProcessingInterface() = default;

        virtual void ProcessFrame(uint32_t height, uint32_t width, uint8_t *address, uint32_t stride) = 0;
    };

} // namespace hawk_camera

#endif