#include <iomanip>
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "HawkCameraController.h"
#include "OpenCVFrameProcessor.h"

using namespace std::literals::chrono_literals;

int main()
{
	hawk_camera::HawkCameraController camController{libcamera::formats::RGB888, 1280, 960};
	std::shared_ptr<hawk_camera::OpenCVFrameProcessor> videoProcessor = std::make_shared<hawk_camera::OpenCVFrameProcessor>();
	cv::namedWindow("IM");

	uint32_t dx = 9152 / 2, dy = 6944 / 2;

	camController.SetViewPortSize(dx, dy);

	camController.Start();

	camController.AddProcessor(videoProcessor);

	while (true)
	{
		if (videoProcessor->HasFrame())
		{
			const cv::Mat mt = videoProcessor->GetLatestFrame();
			if (!mt.empty())
			{
				cv::imshow("IM", mt);
				videoProcessor->RemoveLatestFrame();
			}
			char c = (char)cv::waitKey(1);
			if (c == 27)
				break;
		}
		else
			std::this_thread::sleep_for(30ms);
	}

	cv::destroyAllWindows();

	return 0;
}