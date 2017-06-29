

#include "haar.h"
#include "image.h"
#include <omp.h>
#include <stdio.h>
#include "stdio-wrapper.h"

static int *stages_array;
static int *rectangles_array;
static int *weights_array;
static int *alpha1_array;
static int *alpha2_array;
static int *tree_thresh_array;
static int *stages_thresh_array;
static int **scaled_rectangles_array;
int clock_counter = 0;
int iter_counter = 0;
void integralImages( MyImage *src, MyIntImage *sum, MyIntImage *sqsum );
void ScaleImage_Invoker( myCascade* _cascade, float _factor, int sum_row, int sum_col, std::vector<MyRect>& _vec);
void nearestNeighbor (MyImage *src, MyImage *dst);
inline  int  myRound( float value )
{
  return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

std::vector<MyRect> detectObjects( MyImage* _img, MySize minSize, MySize maxSize, myCascade* cascade1,
				   float scaleFactor, int minNeighbors)
{
  const float GROUP_EPS = 0.4f;
  MyImage *img = _img;
  std::vector<MyRect> allCandidates;
  float factor;

  if( maxSize.height == 0 || maxSize.width == 0 )
    {
      maxSize.height = img->height;
      maxSize.width = img->width;
    }
   MySize winSize0 = cascade1->orig_window_size;
  factor = 1;
  int max=0;
  while(1)
  {
      factor *= scaleFactor;
      max++;
      if((img->width/factor - winSize0.width) < 0 || (img->height/factor - winSize0.height) < 0)
      {
        break;
      }
  }
  int i=0;

  /* Scalability */
  #pragma omp parallel for num_threads(max)
  for(i=0;i<=max;i++)
    {
      myCascade cascadeObj;
      myCascade *cascade = &cascadeObj;
      cascade->n_stages=25;
      cascade->total_nodes=2913;
      cascade->orig_window_size.height = 24;
      cascade->orig_window_size.width = 24;
      MyImage image1Obj;
      MyIntImage sum1Obj;
      MyIntImage sqsum1Obj;
      MyImage *img1 = &image1Obj;
      MyIntImage *sum1 = &sum1Obj;
      MyIntImage *sqsum1 = &sqsum1Obj;
      createImage(img->width, img->height, img1);
      createSumImage(img->width, img->height, sum1);
      createSumImage(img->width, img->height, sqsum1);
      int t=0;
      float fact=1;
      while(t<i)
       { fact *= scaleFactor;
        t++;
       }
      iter_counter++;
      MySize winSize = { myRound(winSize0.width*factor), myRound(winSize0.height*factor) };
      MySize sz = { ( img->width/fact ), ( img->height/fact ) };
      MySize sz1 = { sz.width - winSize0.width, sz.height - winSize0.height };
      if( winSize.width < minSize.width || winSize.height < minSize.height )
	       continue;
     
      setImage(sz.width, sz.height, img1);
      setSumImage(sz.width, sz.height, sum1);
      setSumImage(sz.width, sz.height, sqsum1);
      nearestNeighbor(img, img1);
      integralImages(img1, sum1, sqsum1);
      #pragma omp critical
      {
      setImageForCascadeClassifier( cascade, sum1, sqsum1);
      ScaleImage_Invoker(cascade, fact, sum1->height, sum1->width,
			 allCandidates);
      }
      
      freeImage(img1);
      freeSumImage(sum1);
      freeSumImage(sqsum1);
      
    }

    if( minNeighbors != 0)
      {
        groupRectangles(allCandidates, minNeighbors, GROUP_EPS);
      }
    return allCandidates;
}

unsigned int int_sqrt (unsigned int value)
{
  int i;
  unsigned int a = 0, b = 0, c = 0;
  for (i=0; i < (32 >> 1); i++)
    {
      c<<= 2;
#define UPPERBITS(value) (value>>30)
      c += UPPERBITS(value);
#undef UPPERBITS
      value <<= 2;
      a <<= 1;
      b = (a<<1) | 1;
      if (c >= b)
	{
	  c -= b;
	  a++;
	}
    }
  return a;
}


void setImageForCascadeClassifier( myCascade* _cascade, MyIntImage* _sum, MyIntImage* _sqsum)
{
  MyIntImage *sum = _sum;
  MyIntImage *sqsum = _sqsum;
  myCascade* cascade = _cascade;
  int i, j, k;
  MyRect equRect;
  int r_index = 0;
  int w_index = 0;
  MyRect tr;

  cascade->sum = *sum;
  cascade->sqsum = *sqsum;
  equRect.x = equRect.y = 0;
  equRect.width = cascade->orig_window_size.width;
  equRect.height = cascade->orig_window_size.height;
  cascade->inv_window_area = equRect.width*equRect.height;
  cascade->p0 = (sum->data);
  cascade->p1 = (sum->data +  equRect.width - 1) ;
  cascade->p2 = (sum->data + sum->width*(equRect.height - 1));
  cascade->p3 = (sum->data + sum->width*(equRect.height - 1) + equRect.width - 1);
  cascade->pq0 = (sqsum->data);
  cascade->pq1 = (sqsum->data +  equRect.width - 1) ;
  cascade->pq2 = (sqsum->data + sqsum->width*(equRect.height - 1));
  cascade->pq3 = (sqsum->data + sqsum->width*(equRect.height - 1) + equRect.width - 1);
  for( i = 0; i < cascade->n_stages; i++ )
    {
      for( j = 0; j < stages_array[i]; j++ )
	{
	  int nr = 3;
	  for( k = 0; k < nr; k++ )
	    {
	      tr.x = rectangles_array[r_index + k*4];
	      tr.width = rectangles_array[r_index + 2 + k*4];
	      tr.y = rectangles_array[r_index + 1 + k*4];
	      tr.height = rectangles_array[r_index + 3 + k*4];
	      if (k < 2)
		    {
		      scaled_rectangles_array[r_index + k*4] = (sum->data + sum->width*(tr.y ) + (tr.x )) ;
		      scaled_rectangles_array[r_index + k*4 + 1] = (sum->data + sum->width*(tr.y ) + (tr.x  + tr.width)) ;
		      scaled_rectangles_array[r_index + k*4 + 2] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x ));
		      scaled_rectangles_array[r_index + k*4 + 3] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x  + tr.width));
	     	}
	    else
	   	{
		  if ((tr.x == 0)&& (tr.y == 0) &&(tr.width == 0) &&(tr.height == 0))
		    {
		      scaled_rectangles_array[r_index + k*4] = NULL ;
		      scaled_rectangles_array[r_index + k*4 + 1] = NULL ;
		      scaled_rectangles_array[r_index + k*4 + 2] = NULL;
		      scaled_rectangles_array[r_index + k*4 + 3] = NULL;
		    }
		  else
		    {
		      scaled_rectangles_array[r_index + k*4] = (sum->data + sum->width*(tr.y ) + (tr.x )) ;
		      scaled_rectangles_array[r_index + k*4 + 1] = (sum->data + sum->width*(tr.y ) + (tr.x  + tr.width)) ;
		      scaled_rectangles_array[r_index + k*4 + 2] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x ));
		      scaled_rectangles_array[r_index + k*4 + 3] = (sum->data + sum->width*(tr.y  + tr.height) + (tr.x  + tr.width));
		    }
		    } 
	    } 
	  r_index+=12;
	  w_index+=3;
	} 
  }
}

inline int evalWeakClassifier(int variance_norm_factor, int p_offset, int tree_index, int w_index, int r_index )
{

  /* the node threshold is multiplied by the standard deviation of the image */
  int t = tree_thresh_array[tree_index] * variance_norm_factor;

  int sum = (*(scaled_rectangles_array[r_index] + p_offset)
	     - *(scaled_rectangles_array[r_index + 1] + p_offset)
	     - *(scaled_rectangles_array[r_index + 2] + p_offset)
	     + *(scaled_rectangles_array[r_index + 3] + p_offset))
    * weights_array[w_index];


  sum += (*(scaled_rectangles_array[r_index+4] + p_offset)
	  - *(scaled_rectangles_array[r_index + 5] + p_offset)
	  - *(scaled_rectangles_array[r_index + 6] + p_offset)
	  + *(scaled_rectangles_array[r_index + 7] + p_offset))
    * weights_array[w_index + 1];

  if ((scaled_rectangles_array[r_index+8] != NULL))
    sum += (*(scaled_rectangles_array[r_index+8] + p_offset)
	    - *(scaled_rectangles_array[r_index + 9] + p_offset)
	    - *(scaled_rectangles_array[r_index + 10] + p_offset)
	    + *(scaled_rectangles_array[r_index + 11] + p_offset))
      * weights_array[w_index + 2];

  if(sum >= t)
    return alpha2_array[tree_index];
  else
    return alpha1_array[tree_index];
}

int runCascadeClassifier( myCascade* _cascade, MyPoint pt, int start_stage )
{

  int p_offset, pq_offset;
  int i, j;
  unsigned int mean;
  unsigned int variance_norm_factor;
  int haar_counter = 0;
  int w_index = 0;
  int r_index = 0;
  int stage_sum;
  myCascade* cascade;
  cascade = _cascade;
	
  p_offset = pt.y * (cascade->sum.width) + pt.x;
  pq_offset = pt.y * (cascade->sqsum.width) + pt.x;


  variance_norm_factor =  (cascade->pq0[pq_offset] - cascade->pq1[pq_offset] - cascade->pq2[pq_offset] + cascade->pq3[pq_offset]);
  mean = (cascade->p0[p_offset] - cascade->p1[p_offset] - cascade->p2[p_offset] + cascade->p3[p_offset]);

  variance_norm_factor = (variance_norm_factor*cascade->inv_window_area);
  variance_norm_factor =  variance_norm_factor - mean*mean;


  if( variance_norm_factor > 0 )
    variance_norm_factor = int_sqrt(variance_norm_factor);
  else
    variance_norm_factor = 1;

  for( i = start_stage; i < cascade->n_stages; i++)
    {
      stage_sum = 0;  
      for( j = 0; j < stages_array[i]; j++ )
	     {
	       stage_sum += evalWeakClassifier(variance_norm_factor, p_offset, haar_counter, w_index, r_index);
	       haar_counter++;
	       w_index+=3;
	       r_index+=12;
      }
      if( stage_sum < 0.4*stages_thresh_array[i] ){
	      return -i;
      } 
    }
    return 1;
}

void ScaleImage_Invoker( myCascade* _cascade, float _factor, int sum_row, int sum_col, std::vector<MyRect>& _vec)
{

  myCascade* cascade = _cascade;
  float factor = _factor;
  MyPoint p;
  int result;
  int y1, y2, x2, x, y, step;
  std::vector<MyRect> *vec = &_vec;

  MySize winSize0 = cascade->orig_window_size;
  MySize winSize;

  winSize.width =  myRound(winSize0.width*factor);
  winSize.height =  myRound(winSize0.height*factor);
  y1 = 0;

  y2 = sum_row - winSize0.height;
  x2 = sum_col - winSize0.width;
  step = 1;

  for( x = 0; x <= x2; x += step )
    for( y = y1; y <= y2; y += step )
      {
	     p.x = x;
	     p.y = y;
        result = runCascadeClassifier( cascade, p, 0 );
        if( result > 0 )
	     {
	        MyRect r = {myRound(x*factor), myRound(y*factor), winSize.width, winSize.height};
	        vec->push_back(r);
	     }
    }
}

void integralImages( MyImage *src, MyIntImage *sum, MyIntImage *sqsum )
{
  int x, y;
  unsigned char it;
  int height = src->height;
  int width = src->width;
  unsigned char *data = src->data;
  int * sumData = sum->data;
  int * sqsumData = sqsum->data;

  for( y = 0; y < height; y++)
    {
      int s = 0;
      int sq = 0;
      int  t, tq;
      for( x = 0; x < width; x ++)
	   {
	       it = data[y*width+x];
	       s += it; 
	       sq += it*it;
	       t = s;
      	 tq = sq;
      	 if (y != 0)
      	 {
      	   t += sumData[(y-1)*width+x];
      	   tq += sqsumData[(y-1)*width+x];
      	 }
	       sumData[y*width+x]=t;
	       sqsumData[y*width+x]=tq;
	     }
    }
}

void nearestNeighbor (MyImage *src, MyImage *dst)
{

  int j;
  int i;
  int w1 = src->width;
  int h1 = src->height;
  int w2 = dst->width;
  int h2 = dst->height;
  unsigned char* src_data = src->data;
  unsigned char* dst_data = dst->data;
  int x_ratio = (int)((w1<<16)/w2) +1;
  int y_ratio = (int)((h1<<16)/h2) +1;

  #pragma omp parallel for num_threads(10)
  for (i=0;i<h2;i++)
    {
      unsigned char*  t = dst_data + i*w2;
      int y = ((i*y_ratio)>>16);
      unsigned char* p = src_data + y*w1;
      int rat = 0;
    #pragma omp parallel for num_threads(10) reduction(+:rat)
    for (j=0;j<w2;j++)
	   {
	       int x = (rat>>16);
	       *t++ = p[x];
	       rat += x_ratio;
	   }
    }
}

void readTextClassifier()//(myCascade * cascade)
{
  int stages;
  int total_nodes = 0;
  int i, j, k, l;
  char mystring [12];
  int r_index = 0;
  int w_index = 0;
  int tree_index = 0;
  FILE *finfo = fopen("info.txt", "r");

  if ( fgets (mystring , 12 , finfo) != NULL )
    {
      stages = atoi(mystring);
    }
  i = 0;

  stages_array = (int *)malloc(sizeof(int)*stages);

  while ( fgets (mystring , 12 , finfo) != NULL )
    {
      stages_array[i] = atoi(mystring);
      total_nodes += stages_array[i];
      i++;
    }
  fclose(finfo);

  rectangles_array = (int *)malloc(sizeof(int)*total_nodes*12);
  scaled_rectangles_array = (int **)malloc(sizeof(int*)*total_nodes*12);
  weights_array = (int *)malloc(sizeof(int)*total_nodes*3);
  alpha1_array = (int*)malloc(sizeof(int)*total_nodes);
  alpha2_array = (int*)malloc(sizeof(int)*total_nodes);
  tree_thresh_array = (int*)malloc(sizeof(int)*total_nodes);
  stages_thresh_array = (int*)malloc(sizeof(int)*stages);
  FILE *fp = fopen("class.txt", "r");


  for (i = 0; i < stages; i++)
    {   
      for (j = 0; j < stages_array[i]; j++)
	     {	
	         for(k = 0; k < 3; k++)
	         {	
	           for (l = 0; l <4; l++)
		        {
		          if (fgets (mystring , 12 , fp) != NULL)
		              rectangles_array[r_index] = atoi(mystring);
        		  else
        		    break;
        		    r_index++;
        		}
        	   if (fgets (mystring , 12 , fp) != NULL)
        		{
        		  weights_array[w_index] = atoi(mystring);
        		}
        	   else
        		    break;
        	   w_index++;
        	 } 
        	  if (fgets (mystring , 12 , fp) != NULL)
        	    tree_thresh_array[tree_index]= atoi(mystring);
        	  else
        	    break;
        	  if (fgets (mystring , 12 , fp) != NULL)
        	    alpha1_array[tree_index]= atoi(mystring);
        	  else
        	    break;
        	  if (fgets (mystring , 12 , fp) != NULL)
        	    alpha2_array[tree_index]= atoi(mystring);
        	  else
        	    break;
        	  tree_index++;
        	  if (j == stages_array[i]-1)
        	    {
        	      if (fgets (mystring , 12 , fp) != NULL)
        		stages_thresh_array[i] = atoi(mystring);
        	      else
        		break;
        	    }
	       } 
      } 
  fclose(fp);
}

void releaseTextClassifier()
{
  free(stages_array);
  free(rectangles_array);
  free(scaled_rectangles_array);
  free(weights_array);
  free(tree_thresh_array);
  free(alpha1_array);
  free(alpha2_array);
  free(stages_thresh_array);
}
/* End of file. */
