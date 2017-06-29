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

#define INPUT_FILENAME "Face.pgm"
#define OUTPUT_FILENAME "Output7.jpg"
using namespace cv;
using namespace std;

int main (int argc, char *argv[]) 
{	
	IplImage* img_gray = cvLoadImage("1.jpg",CV_LOAD_IMAGE_GRAYSCALE);
	cvSaveImage("Face.pgm",img_gray); 
	int flag;
	int mode = 1;
	int i;
	float scaleFactor = 1.5;
	int minNeighbours = 1;
	MyImage imageObj;
	MyImage *image = &imageObj;
	flag = readPgm((char *)"Face.pgm", image);
	if (flag == -1)
	{
		printf( "Unable to open input image\n");
		return 1;
	}
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
	double time=omp_get_wtime();
	result = detectObjects(image, minSize, maxSize, cascade, scaleFactor, minNeighbours);
	for(i = 0; i < result.size(); i++ )
	{
		MyRect r = result[i];
		drawRectangle(image, r);
       
	}
	//printf("Face Detected :- %d", result.size()); 
	printf("\nActual time of Execution(Without Read & Write) :-  %f\n",(omp_get_wtime()-time));
	flag = writePgm((char *)OUTPUT_FILENAME, image); 
	releaseTextClassifier();
	freeImage(image);
	
	return 0;
}
