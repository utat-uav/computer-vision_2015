// Surf Image Detection
// Author: Winston Liu 
// For UTAT use only.

#include <opencv2/features2d/features2d.hpp>  
#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "dbscan.h"

// EXPERIMENTAL TEST PROGRAM ONLY
// >> IF MAIN.CPP EXISTS, USE THAT INSTEAD. THIS IS FOR 

// Log:
//
// 1.jpg works with threshold 15000, doesn't work with 2 or 3
// fastnlmeans -> no improvement, still 13 keypoints.
// EUREKA! Everything works with AC1 -> filter by hue

// Steps:
// 1. Convert to HSV
// 2. Blur
// 3. Run SURF
// 4. Run DBSCAN
// 5. Foreach cluster, check validity and output

#define DEBUG_ON
#define ACTIVE_CHANNEL 1

float eps = 200;
int minPts = 2;
// ROI is at most 1/10 of full-sized image
int roiProportional = 10;

cv::Point2i getMean(std::vector<cv::KeyPoint>& subsetKeys)
{
	if (subsetKeys.empty() == true)
		return cv::Point2i(0, 0);
	cv::Point2i meanpt(0, 0);
	int greatest_dist = 0;
	int maxsize = 0;

	// Calculate mean
	for (int i = 0; i < subsetKeys.size(); ++i) 
	{ 
		meanpt.x += subsetKeys[i].pt.x; 
		meanpt.y += subsetKeys[i].pt.y; 

#ifdef DEBUG_ON
		std::cout << "~~ x: ";
		std::cout << subsetKeys[i].pt.x;
		std::cout << " y: ";
		std::cout << subsetKeys[i].pt.y << std::endl;
#endif

	}
	meanpt.x /= subsetKeys.size();
	meanpt.y /= subsetKeys.size();

#ifdef DEBUG_ON
	std::cout << ">>> key pt. avg: " << meanpt.x << " , ";
	std::cout << meanpt.y << std::endl;
#endif

	return meanpt;
}

//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << 
			"./image_proc <file> <thresh> (-1 for default)" << std::endl;
		return -1;
	}

	int thresh;
	std::vector<std::vector<cv::KeyPoint> > clusters;

	// This usually works
	if (strcmp(argv[2], "-1") == true)
		thresh = 4000;
	else
		thresh = atoi(argv[2]);

	//std::cout << thresh << std::endl;

	// Declarations
	cv::Mat test_im = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
	cv::Mat hsvim, outim, channels[3], roiIm, roiChan[3], roiOut;
	std::vector<cv::KeyPoint> keypoints, roiPoints;

	cv::Mat hsvchannel;

	// Convert to HSV
	std::cout << "Converting to HSV ..." << std::endl;
	try
	{
		cv::cvtColor(test_im, hsvim, CV_RGB2HSV); 
		cv::split(hsvim, channels);
		std::cout << "Saving image ..." << std::endl;
		cv::imwrite("hsvim.jpg", hsvim);
		hsvchannel = channels[ACTIVE_CHANNEL];
	}
	catch (...)
	{
		std::cout << "Failed." <<  std::endl;
		return -1;
	}

	// Denoise
	std::cout << "Blurring ..." << std::endl;
	//cv::fastNlMeansDenoising(hsvchannel, hsvchannel, 10);
	//cv::medianBlur(hsvchannel, hsvchannel, 27);
	try
	{
		cv::blur(hsvchannel, hsvchannel, cv::Size(10,10));
		std::cout << "Saving image ..." << std::endl;
		cv::imwrite("blur.jpg", hsvim);
	}
	catch (...)
	{
		std::cout << "Failed." <<  std::endl;
		return -1;
	}

	// Using surf
	std::cout << "Running SURF ..." << std::endl;
	// cv::SurfFeatureDetector medetect(thresh);
	// medetect.detect(hsvchannel, keypoints);

	// not using feature detector
	// Default is ( ~, 4, 2, true, false)
	// See http://docs.opencv.org/modules/nonfree/doc/feature_detection.html
	try
	{
		cv::Mat mask;
		cv::SURF mysurf(thresh, 4, 2, true, true);
		mysurf(hsvchannel, mask, keypoints);

		std::cout << "Saving image ..." << std::endl;
		cv::imwrite("surf.jpg", hsvim);
	}
	catch (...)
	{
		std::cout << "Failed." <<  std::endl;
		return -1;
	}

	// Print keypoints
	std::cout << "Drawing keypoints ..." << std::endl;
	try
	{
		cv::drawKeypoints(hsvchannel, keypoints, outim, (0, 255, 0), 4);
		std::cout << "Number of keypoints: " << keypoints.size() << std::endl;
		std::cout << "Saving image ..." << std::endl;
		cv::imwrite("keypoints.jpg", hsvim);
	}
	catch (...)
	{
		std::cout << "Failed." <<  std::endl;
		return -1;
	}


	// DBSCAN actual
	std::cout << "Running DBSCAN ..." << std::endl;
	try
	{
		clusters = DBSCAN_keypoints(&keypoints, eps, minPts);
		std::cout << "Cluster size: " << clusters.size() << std::endl;
	}
	catch (...)
	{
		std::cout << "Failed." <<  std::endl;
		return -1;
	}

	for (int i = 0; i < clusters.size(); ++i)
	{
		try
		{
			std::ostringstream oss;
			std::cout << "::Processing cluster " << i << std::endl;

			// Calculate mean
			cv::Point2i mean;
			cv::Point2i roi;
			std::cout << ":::Getting cluster mean" << std::endl;
			mean = getMean(clusters[i]);

			if (mean.x == 0 && mean.y == 0)
				continue;

			// Get ROI size as function of image size
			roi.x = hsvchannel.cols / roiProportional;
			roi.y = hsvchannel.rows / roiProportional;

			// Set mean coordinates to the upper left corner
			mean.x -= roi.x / 2;
			mean.y -= roi.y / 2;

			// Clamp to image range
			if (mean.x < 0)
				mean.x = 0;
			else if (mean.x + roi.x > hsvchannel.cols) 
				roi.x = hsvchannel.cols - mean.x;

			if (mean.y < 0)
				mean.y = 0;
			else if (mean.y + roi.y > hsvchannel.cols) 

				roi.y = hsvchannel.cols - mean.y;

			// Name image
			std::string raw_name(argv[1]); 
			int firstindex = raw_name.find_last_of("/");
			int lastindex = raw_name.find_last_of(".");
			std::string proc_name = raw_name.substr(firstindex + 1, lastindex);

			std::cout << "ROI_" << proc_name << i << ".jpg" << std::endl;
			oss << "ROI_" << proc_name << "_" << i << ".jpg";

			roiIm = hsvim(cv::Rect(mean.x, mean.y, roi.x, roi.y));
			cv::split(roiIm, roiChan);

			// Using FAST edge detection
			/*
			   std::cout << "Edge detection ..." << std::endl;
			   cv::FAST(roiChan[ACTIVE_CHANNEL], roiPoints, 100);	
			   */

			std::cout << "Drawing keypoints ..." << std::endl;
			cv::drawKeypoints(roiChan[ACTIVE_CHANNEL], roiPoints, roiIm);

			// Display ROI
			roiOut = test_im(cv::Rect(mean.x, mean.y, roi.x, roi.y));
			std::cout << "Saving roi image ..." << std::endl;
			cv::imwrite(oss.str(), roiOut);
		}
		catch (...)
		{
			std::cout << "An error occured." << std::endl;
		}
	}

	std::cout << "Saving image ..." << std::endl;
	cv::imwrite("imout.jpg", outim);

	//cv::waitKey(0);
	std::cout << "Done." << std::endl;
	return 0;
}
