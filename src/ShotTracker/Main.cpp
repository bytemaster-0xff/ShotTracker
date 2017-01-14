#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>

using namespace cv;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

//![variables]
Mat src, src_gray;
Mat dst, detected_edges;

int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
const char* window_name = "Edge Map";


/**
* @function CannyThreshold
* @brief Trackbar callback - Canny thresholds input with a ratio 1:3
*/
static void CannyThreshold(int, void*)
{
	//![reduce_noise]
	/// Reduce noise with a kernel 3x3
	blur(src_gray, detected_edges, Size(3, 3));
	//![reduce_noise]

	//![canny]
	/// Canny detector
	Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);
	//![canny]

	/// Using Canny's output as a mask, we display our result
	//![fill]
	dst = Scalar::all(0);
	//![fill]

	//![copyto]
	src.copyTo(dst, detected_edges);
	//![copyto]

	//![display]
	imshow(window_name, dst);
	//![display]
}

int main(int, char** argv)
{
	std::streampos fileSize;

	//![load]
	src = imread(argv[1], IMREAD_COLOR); // Load an image

	uint8_t *buffer;
/*
	std::ifstream file(argv[1], std::ios::binary);
	fileSize = file.tellg();
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<BYTE> fileData(fileSize);
	file.read((char*)&fileData[0], fileSize);

	// read the data:
	src = Mat(fileSize, 1, CV_8U, fileData.data).clone();
	*/

	if (src.empty())
	{
		return -1;
	}
	//![load]

	//![create_mat]
	/// Create a matrix of the same type and size as src (for dst)
	dst.create(src.size(), src.type());
	//![create_mat]

	//![convert_to_gray]
	cvtColor(src, src_gray, COLOR_BGR2GRAY);
	//![convert_to_gray]

	//![create_window]
	namedWindow(window_name, WINDOW_AUTOSIZE);
	//![create_window]

	//![create_trackbar]
	/// Create a Trackbar for user to enter threshold
	createTrackbar("Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold);
	//![create_trackbar]

	/// Show the image
	CannyThreshold(0, 0);

	//StartListening();

	/// Wait until user exit program by pressing a key
	waitKey(0);

	return 0;
}