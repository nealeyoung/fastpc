#include "/usr/include/opencv/cv.h"
#include "/usr/include/opencv/highgui.h"
#include <stdio.h>


int main(){

  int i,j,k;

  int height = 0;
  int width = 0;
  int step = 0;
  int channels = 0;
  IplImage* img;
  uchar *data;
  img= (IplImage*)cvLoadImage("test2.jpg",0);
  //printf("height: %d \n",img->height);
  data =(uchar *)img->imageData;
  width     = img->width;
  step      = img->widthStep;
  channels  = img->nChannels;
  printf("Processing a %dx%d image with %d channels\n",height,width,channels); 
  //printf("%d %d %d \n", width, step, channels);
  for(i=0;i<height;i++) 
    for(j=0;j<width;j++) 
      for(k=0;k<channels;k++)
	data[i*step+j*channels+k]=0;//-data[i*step+j*channels+k];
  //  for(i=0;i<height;i++) for(j=0;j<width;j++)
  //  data[i*step+j]=255-data[i*step+j];

  img->imageData = data;

  cvSaveImage("opencv_out.png",img);
}
