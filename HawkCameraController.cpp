#include "HawkCameraController.h"
#include <sys/mman.h>
#include <unistd.h>

namespace hawk_camera
{

    HawkCameraController::HawkCameraController(const libcamera::PixelFormat &format, uint32_t width, uint32_t height) : m_scalerCropChanged(false)
    {
        m_cm.start();

        const auto camList = m_cm.cameras();
        if (camList.empty())
        {
            Log() << "Cant find active camera" << std::endl;
            return;
        }
        const std::string cameraId = camList[0]->id();
        m_camera = m_cm.get(cameraId);
        m_camera->acquire();

        std::unique_ptr<libcamera::CameraConfiguration> configs =
            m_camera->generateConfiguration({libcamera::StreamRole::Raw});

        if (configs->empty())
        {
            Log() << "Empty camera config" << std::endl;
            m_camera.reset();
            return;
        }

        auto &config = configs->at(0);
        config.pixelFormat = format;
        config.size.width = width;
        config.size.height = height;
        config.bufferCount = 1;

        if (configs->validate() == libcamera::CameraConfiguration::Status::Invalid)
        {
            Log() << "Wrong config" << std::endl;
            m_camera.reset();
            return;
        }

        m_camera->configure(configs.get());

        m_allocator = std::make_unique<libcamera::FrameBufferAllocator>(m_camera);

        for (libcamera::StreamConfiguration &cfg : *configs)
        {
            int ret = m_allocator->allocate(cfg.stream());
            if (ret < 0)
            {
                Log() << "Can't allocate buffers" << std::endl;
                m_camera.reset();
                return;
            }
        }

        m_stream = config.stream();
        const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = m_allocator->buffers(m_stream);
        if (buffers.size() != 1)
        {
            Log() << "Wrong buffers number" << std::endl;
        }

        m_request = m_camera->createRequest();
        if (!m_request)
        {
            Log() << "Can't create request" << std::endl;
            m_camera.reset();
            return;
        }

        int ret = m_request->addBuffer(m_stream, buffers[0].get());
        if (ret < 0)
        {
            Log() << "Can't set buffer for request" << std::endl;
            m_camera.reset();
            return;
        }

        SetDefaultControls();

        m_camera->requestCompleted.connect(this, &HawkCameraController::OnRequestComplete);
        m_camera->start();
    }

    HawkCameraController::~HawkCameraController()
    {
        m_autoRestartFlag = false;
        if (m_camera)
        {
            m_camera->stop();
            m_camera->release();
            m_allocator->free(m_stream);
            m_allocator.reset();
            m_camera.reset();
        }

        m_cm.stop();
    }

    void HawkCameraController::Start(bool autoRestartFlag)
    {
        m_autoRestartFlag = autoRestartFlag;
        m_camera->queueRequest(m_request.get());
        m_autoRestartFlag = true;
    }

    void HawkCameraController::SetDefaultControls()
    {
        libcamera::ControlList &controls = m_request->controls();
        controls.set(libcamera::controls::AeEnable, true);
        controls.set(libcamera::controls::AwbEnable, true);

        controls.set(libcamera::controls::AwbMode, libcamera::controls::AwbModeEnum::AwbIndoor);
        controls.set(libcamera::controls::AfMode, libcamera::controls::AfModeEnum::AfModeAuto);
        controls.set(libcamera::controls::AfRange, libcamera::controls::AfRangeEnum::AfRangeNormal);
        // controls.set(libcamera::controls::Brightness, 0.3);
        // controls.set(libcamera::controls::Contrast, 0.5);
        // controls.set(libcamera::controls::ExposureTime, 30000);
    }

    void HawkCameraController::SetViewPort(uint32_t center_X, uint32_t center_Y, uint32_t width, uint32_t height)
    {
        libcamera::ControlList &controls = m_request->controls();
        libcamera::Rectangle scalerCrop(center_X, center_Y, width, height);
        if (CheckViewPort(scalerCrop))
        {
            m_scalerCrop = scalerCrop;
            m_scalerCropChanged = true;
        }
    }

    void HawkCameraController::SetViewPortOrigin(uint32_t center_X, uint32_t center_Y)
    {
        auto newScalerCrop = m_scalerCrop;
        newScalerCrop.x = center_X - m_scalerCrop.width / 2;
        newScalerCrop.y = center_Y - m_scalerCrop.height / 2;
        if (CheckViewPort(newScalerCrop))
        {
            m_scalerCrop = newScalerCrop;
            m_scalerCropChanged = true;
        }
    }

    void HawkCameraController::SetViewPortSize(uint32_t width, uint32_t height)
    {
        auto newScalerCrop = m_scalerCrop;
        newScalerCrop.width = width;
        newScalerCrop.height = height;
        if (CheckViewPort(newScalerCrop))
        {
            m_scalerCrop = newScalerCrop;
            m_scalerCropChanged = true;
        }
    }

    bool HawkCameraController::CheckViewPort(const libcamera::Rectangle &rect)
    {
        return rect.topLeft().x >= 0 && rect.topLeft().x + rect.size().width <= k_matrixWidth &&
               rect.topLeft().y >= 0 && rect.topLeft().y + rect.size().height <= k_matrixHeight;
    }

    void HawkCameraController::AddProcessor(std::shared_ptr<FrameProcessingInterface> proc)
    {
        std::lock_guard<std::mutex> lck{m_procMutex};
        m_processors.emplace_back(proc);
    }

    void HawkCameraController::OnRequestComplete(libcamera::Request *request)
    {
        if (request->status() == libcamera::Request::RequestCancelled)
            return;

        const libcamera::Request::BufferMap &buffers = request->buffers();
        for (auto &[stream, buffer] : buffers)
        {
            const auto &cfg = stream->configuration();
            const auto width = cfg.size.width;
            const auto height = cfg.size.height;
            const auto stride = cfg.stride;
            const auto pixelFormat = cfg.pixelFormat;

            // Log() << " size " << width << "x" << height << " stride " << stride << " format " << cfg.pixelFormat.toString() << " sec "
            //       << (double)clock() / CLOCKS_PER_SEC << std::endl;

            if (buffer->planes().size() > 0)
            {
                const libcamera::FrameBuffer::Plane &plane = buffer->planes()[0];
                const int fd = plane.fd.get();
                const size_t length = ::lseek(fd, 0, SEEK_END);
                size_t mapLength = static_cast<size_t>(plane.offset + plane.length);
                void *address = ::mmap(nullptr, mapLength, PROT_READ, MAP_SHARED, fd, 0);
                if (address != MAP_FAILED)
                {
                    std::lock_guard<std::mutex> lck{m_procMutex};
                    for (std::shared_ptr<FrameProcessingInterface> proc : m_processors)
                    {
                        proc->ProcessFrame(width, height, reinterpret_cast<uint8_t *>(address), stride);
                    }
                    ::munmap(address, mapLength);
                }
            }

            // libcamera::ControlList &controls = request->controls();
            // auto lens = controls.get(libcamera::controls::FocusFoM);
            // auto pos = lens.value();

            if (m_autoRestartFlag)
            {
                request->reuse(libcamera::Request::ReuseBuffers);
                if (m_scalerCropChanged)
                {
                    libcamera::ControlList &controls = m_request->controls();
                    controls.set(libcamera::controls::ScalerCrop, m_scalerCrop);
                    m_scalerCropChanged = false;
                }
                m_camera->queueRequest(request);
            }
        }
    }

} // hawk_camera
