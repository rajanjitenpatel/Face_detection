#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>

#include "image.h"
#include "stdio-wrapper.h"
#include "haar.h"

#define INPUT_FILENAME "1.jpg"
#define OUTPUT_FILENAME "output.jpg"
using namespace cv;
using namespace std;



int main (int argc, char *argv[]) 
{

	IplImage *frame;
  	IplImage *img_gray;
  	#pragma omp parallel sections num_threads(2)
  	{
  		#pragma omp section
  		{
			frame = cvLoadImage(INPUT_FILENAME,CV_LOAD_IMAGE_COLOR);
		}
		#pragma omp section
		{
    		img_gray = cvLoadImage("1.jpg",CV_LOAD_IMAGE_GRAYSCALE);
    	}
	}
	int flag;
	int mode = 1;
	int i;
	float scaleFactor = 1.5;
	int minNeighbours = 1;
	MyImage imageObj;
	MyImage *image = &imageObj;
	image->width = img_gray->width;
	image->height = img_gray->height; 
	image->maxgrey = 255;
	image->data = (unsigned char*)img_gray->imageData;
	image->flag = 1;
	myCascade cascadeObj;
	myCascade *cascade = &cascadeObj;
	MySize minSize = {20, 20};
	MySize maxSize = {0, 0};
	cascade->n_stages=25;
	cascade->total_nodes=2913;
	cascade->orig_window_size.height = 24;
	cascade->orig_window_size.width = 24;
	readTextClassifier();
	std::vector<MyRect> result;
	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int   lineWidth=1;
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, hScale,vScale,1,lineWidth);
	double time=omp_get_wtime();
	//omp_set_nested(1);


	result = detectObjects(image, minSize, maxSize, cascade, scaleFactor, minNeighbours);
   	

   	#pragma omp parallel for num_threads(result.size())
	for(i = 0; i < result.size(); i++ )
	{
		MyRect r = result[i];
		cvRectangle(frame,cvPoint(r.x,r.y),cvPoint(r.x + r.width, r.y + r.height),CV_RGB(243,243,21), 4, 8, 0);
	}
	printf("Face Detected :- %d",result.size()); 
	printf("\nActual Time of Execution(Without Read & Write) :-  %f\n",(omp_get_wtime()-time));
	cvSaveImage(OUTPUT_FILENAME,frame);
	releaseTextClassifier();
	return 0;
}
