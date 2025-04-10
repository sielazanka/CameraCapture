#include<opencv2/core.hpp>
#include<opencv2/opencv.hpp>
#include<iostream>
#include<shared_mutex>
#include<thread>
#include<atomic>

class CameraCapture {
private:
	cv::VideoCapture cap_;
	cv::Mat frame_;
	unsigned int id_;

	std::shared_mutex frameMutex_;
	std::thread captureThread_;
	std::atomic<bool> keepRunning_;
	std::atomic<bool> stopped_;

	void mainLoop() {
		cap_.open(id_);
		if (!cap_.isOpened()) {
			std::cerr << "Console error" << std::endl;
			return;
		}
		while (keepRunning_) {
			cv::Mat temp;
			cap_.read(temp);
			if (temp.empty()) {
				continue;
			}
			std::unique_lock<std::shared_mutex> lock(frameMutex_);
			frame_ = std::move(temp);
		}
	}

	void start() {
		captureThread_ = std::thread(&CameraCapture::mainLoop, this);
	}

public:
	CameraCapture(int id) :id_(id), keepRunning_(true), stopped_(false) {
		start();
	}

	CameraCapture(const CameraCapture& other) = delete;
	CameraCapture(CameraCapture&& other) = delete;
	CameraCapture& operator=(const CameraCapture& other) = delete;
	CameraCapture& operator=(CameraCapture&& other) = delete;

	~CameraCapture() {
		stop();
	}

	void stop() {
		if (stopped_) {
			return;
		}
		stopped_ = true;
		keepRunning_ = false;

		if (captureThread_.joinable()) {
			captureThread_.join();
		}

		if (cap_.isOpened()) {
			cap_.release();
		}
	}

	cv::Mat getFrame() {
		std::shared_lock<std::shared_mutex> lock(frameMutex_);
		return frame_.clone();
	}
};

int main() {
	CameraCapture cam(0);
	while (true) {
		auto frame = cam.getFrame();
		if (!frame.empty()) {
			cv::imshow("frame", frame);
		}

		auto key = cv::waitKey(1);
		if (key == 'q') {
			break;
		}
	}

	cam.stop();
	cv::destroyAllWindows();
}