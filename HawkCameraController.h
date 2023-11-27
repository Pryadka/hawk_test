#ifndef __HAWK_CAMERA__
#define __HAWK_CAMERA__

#include <libcamera/libcamera.h>
#include <memory>
#include <vector>
#include <mutex>
#include <iostream>
#include "FrameProcessingInterface.h"

namespace hawk_camera
{

  class HawkCameraController
  {
  public:
    HawkCameraController(const libcamera::PixelFormat &format, uint32_t width, uint32_t height);
    ~HawkCameraController();

    void Start(bool autoRestartFlag = true);
    void SetViewPort(uint32_t center_X, uint32_t center_Y, uint32_t width, uint32_t height);
    void SetViewPortOrigin(uint32_t center_X, uint32_t center_Y);
    void SetViewPortSize(uint32_t width, uint32_t height);

    void AddProcessor(std::shared_ptr<FrameProcessingInterface> proc);

  private:
    libcamera::CameraManager m_cm;
    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    libcamera::Stream *m_stream = nullptr;
    std::unique_ptr<libcamera::Request> m_request;
    bool m_autoRestartFlag;

    std::vector<std::shared_ptr<FrameProcessingInterface>> m_processors;
    std::mutex m_procMutex;

    void OnRequestComplete(libcamera::Request *request);
    void SetDefaultControls();
    static bool CheckViewPort(const libcamera::Rectangle &rect);
    std::ostream &Log() const { return std::cout; }

    static constexpr size_t k_matrixWidth = 9152;
    static constexpr size_t k_matrixHeight = 6944;
  };

} // hawk_camera

#endif