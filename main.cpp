#include <iomanip>
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "HawkCameraController.h"
#include "OpenCVFrameProcessor.h"

using namespace std::literals::chrono_literals;

int main()
{
	enum Keys
	{
		Left = 81,
		Up = 82,
		Right = 83,
		Down = 84,
	};

	hawk_camera::HawkCameraController camController{libcamera::formats::RGB888, 1280, 960};
	std::shared_ptr<hawk_camera::OpenCVFrameProcessor> videoProcessor = std::make_shared<hawk_camera::OpenCVFrameProcessor>();
	cv::namedWindow("IM");

	uint32_t x_pos = 9152 / 2, y_pos = 6944 / 2; // in the center
	uint32_t view_port_x = 9152 / 2, view_port_y = 6944 / 2;

	camController.SetViewPortSize(view_port_x, view_port_y); // half of the matric
	camController.SetViewPortOrigin(x_pos, y_pos);			 // center of the matrix

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
			int k = cv::waitKey(1);
			if (static_cast<char>(k) == 27)
				break;

			bool move = false;
			bool scale = false;

			switch (k)
			{
			case Keys::Down:
				move = true;
				y_pos += 200;
				break;
			case Keys::Up:
				move = true;
				y_pos -= 200;
				break;
			case Keys::Left:
				move = true;
				x_pos -= 200;
				break;
			case Keys::Right:
				move = true;
				x_pos += 200;
				break;
			case '+':
				scale = true;
				view_port_y += 200;
				view_port_x += 200;
				break;
			case '-':
				scale = true;
				view_port_y -= 200;
				view_port_x -= 200;
				break;
			}
			if (move)
			{
				camController.SetViewPortOrigin(x_pos, y_pos);
			}
			if (scale)
			{
				camController.SetViewPortSize(view_port_x, view_port_y);
			}
			if (move || scale)
			{
				std::cout << "Center(X,Y)=(" << x_pos << "," << y_pos << "), vieport on matrix=(W,H)=(" << view_port_x << "," << view_port_y << ")" << std::endl;
			}
		}
		else
			std::this_thread::sleep_for(30ms);
	}

	cv::destroyAllWindows();

	return 0;
}