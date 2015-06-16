#include <opencv2/features2d/features2d.hpp>  
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <iostream>

#include "dbscan.h"

// Log:
//
// 1.jpg works with threshold 15000, doesn't work with 2 or 3
// fastnlmeans -> no improvement, still 13 keypoints.
// EUREKA! Everything works with AC1 -> filter by hue


#define ACTIVE_CHANNEL 1

const int BLURSIZE = 5;

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

  std::cout << thresh << std::endl;

  cv::Mat test_im = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
  cv::Mat hsvim, outim, channels[3], roiChan[3];
	cv::Mat cannyOut;
  std::vector<cv::KeyPoint> keypoints, roiPoints;
	std::vector<std::vector<cv::Point> > mycontour;
	std::vector< cv::Vec4i > hierarchy;
  cv::KeyPoint featurePoint;

  // Convert to HSV
  std::cout << "Converting to HSV ..." << std::endl;
  cv::cvtColor(test_im, hsvim, CV_RGB2HSV); 
  cv::split(hsvim, channels);

  cv::Mat hsvchannel = channels[ACTIVE_CHANNEL];
 	cv::blur(hsvchannel, hsvchannel, cv::Size(BLURSIZE,BLURSIZE));

	std::cout << "Edge detection ..." << std::endl;
	cv::Canny(hsvchannel, cannyOut, thresh, thresh*2);
	cv::findContours(cannyOut, mycontour, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	/*
  std::cout << "Number of keypoints: " << roiPoints.size() << std::endl;
	std::cout << "Drawing keypoints ..." << std::endl;
	cv::drawKeypoints(hsvchannel, roiPoints, outim);
	*/

	// Check for closed contours
  std::cout << "Number of contours: " << mycontour.size() << std::endl;
	for (int i = 0; i < mycontour.size(); ++i)
	{
		cv::Rect r = cv::boundingRect(mycontour[i]);
		if(hierarchy[i][2]<0) //Check if there is a child contour
			cv::rectangle(test_im,cv::Point(r.x-10,r.y-10), cv::Point(r.x+r.width+10,r.y+r.height+10), cv::Scalar(255,0,0),1,8,0); //Opened contour
		else
			cv::rectangle(test_im,cv::Point(r.x-10,r.y-10), cv::Point(r.x+r.width+10,r.y+r.height+10), cv::Scalar(0,255,0),2,8,0); //closed contour
	}

  std::cout << "Saving image ..." << std::endl;
  cv::imwrite("imout.jpg", test_im);

  //cv::waitKey(0);
  std::cout << "Done." << std::endl;
  return 0;
}
