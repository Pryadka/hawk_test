#include <iomanip>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include "HawkCameraController.h"
#include "OpenCVFrameProcessor.h"

using namespace std::literals::chrono_literals;
namespace fs = std::filesystem;

bool deleteAndCreateDataFolder()
{
	auto dataPath = fs::current_path() / "images";
	if (fs::exists(dataPath))
	{
		fs::remove_all(dataPath);
	}
	return fs::create_directory(dataPath);
}

void saveImage(int N, int x, int y, const cv::Mat &image)
{
	auto dataPath = fs::current_path() / "images" / std::to_string(N);
	if (!fs::exists(dataPath))
	{
		fs::create_directory(dataPath);
	}
	dataPath /= std::to_string(y + 1) + "_" + std::to_string(x + 1) + ".bmp";
	const std::string fileName = dataPath.string();

	cv::imwrite(fileName, image);
}

void saveImages(hawk_camera::HawkCameraController &camController, std::shared_ptr<hawk_camera::OpenCVFrameProcessor> videoProcessor)
{
	if (!deleteAndCreateDataFolder())
	{
		std::cout << "Cant create directory for saving images" << std::endl;
		return;
	}

	for (int i = 1; i <= 4; i++)
	{
		const int frameSizeX = hawk_camera::HawkCameraController::k_matrixWidth / i;
		const int frameSizeY = hawk_camera::HawkCameraController::k_matrixHeight / i;
		const int centerShiftX = frameSizeX / 2;
		const int centerShiftY = frameSizeY / 2;

		camController.SetViewPortSize(frameSizeX, frameSizeY);
		for (int nx = 0; nx < i; nx++)
		{
			for (int ny = 0; ny < i; ny++)
			{
				while (videoProcessor->HasFrame())
					videoProcessor->RemoveLatestFrame();
				camController.SetViewPortOrigin(centerShiftX + nx * frameSizeX, centerShiftY + ny * frameSizeY);
				camController.Start(false);
				while (!videoProcessor->HasFrame())
				{
					std::this_thread::sleep_for(10ms);
				}
				const cv::Mat mt = videoProcessor->GetLatestFrame();
				saveImage(i, nx, ny, mt);
				videoProcessor->RemoveLatestFrame();
			}
		}
	}
}

int main()
{
	enum Keys
	{
		Left = 81,
		Up = 82,
		Right = 83,
		Down = 84,
		Exit = 27,
		Enter = 13,
	};

	hawk_camera::HawkCameraController camController{libcamera::formats::RGB888, 1280, 960};
	std::shared_ptr<hawk_camera::OpenCVFrameProcessor> videoProcessor = std::make_shared<hawk_camera::OpenCVFrameProcessor>();
	cv::namedWindow("IM");

	camController.SetViewPortSize(hawk_camera::HawkCameraController::k_matrixWidth,
								  hawk_camera::HawkCameraController::k_matrixHeight); // half of the matric
	camController.SetViewPortOrigin(hawk_camera::HawkCameraController::k_matrixWidth / 2,
									hawk_camera::HawkCameraController::k_matrixHeight / 2); // center of the matrix

	camController.Start(false);

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
			camController.Start(false);
		}
		else
		{
			const auto k = cv::waitKey(1);
			if (k == Keys::Exit)
				break;

			switch (k)
			{
			case Keys::Enter:
				saveImages(camController, videoProcessor);
				camController.SetViewPort(0, 0,
										  hawk_camera::HawkCameraController::k_matrixWidth,
										  hawk_camera::HawkCameraController::k_matrixHeight);
				camController.Start(false);
				break;
			default:
				// std::cout << "KeyCode=" << k << std::endl;
				break;
			}
		}
	}

	cv::destroyAllWindows();

	return 0;
}