#include "../include/conv2d_cv2.hpp"
//to-do: conv2d class extending fpga

int width = 224;
int kernel = 5;
int border = kernel/2;
int sizeWithBorder = width + 2*border;
int pixels = sizeWithBorder * sizeWithBorder;

mutex respLock, srcLock;
vector<Mat> respImg(3);
Mat respImgBig = Mat::zeros(sizeWithBorder*3, sizeWithBorder*3, CV_32FC3);
Mat srcImg;

pthread_t tShow;
pthread_attr_t attr2;
bool run = 1;

void showJobResp(commFPGA *fpga, jobResponse *res) {
  
  Mat im = Mat::zeros(sizeWithBorder, sizeWithBorder, CV_32FC1);
  for(int i = 0; i < sizeWithBorder; i++) {
    for(int j = 0; j < sizeWithBorder; j++) {
      int32_t val = (res->payload[i*sizeWithBorder + j]);
      im.at<float>(i, j) = 1.0 * val / (256 - 1);
    }
  }

  respLock.lock();
  respImg[res->tag] = im;
  merge(respImg, respImgBig);
  resize(respImgBig, respImgBig, cv::Size(sizeWithBorder*3,sizeWithBorder*3), 0, 0, CV_INTER_AREA);
  respLock.unlock();
  printf("%16s id %08X\n", fpga->ip, *res->jobId);
}

void *showThread(void *ref) {
  namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
  while(run && waitKey(16) != 27) {
    srcLock.lock();
    if(!srcImg.empty())
      imshow( "Display window", srcImg );
    srcLock.unlock();
    respLock.lock();
    if(!respImgBig.empty())
      imshow( "result", respImgBig);
    respLock.unlock();
  }
  pthread_exit(NULL);
}

int main_conv2d() {
  connection_init();
  usleep(5000);
  
  pthread_attr_init(&attr2);
  pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE);

  pthread_create(&tShow, &attr2, showThread, 0);
  pthread_setname_np(tShow, "tShow");

  VideoCapture stream("sample.mp4");
  //VideoCapture stream(-1);
  if( !stream.isOpened() ) {
    fprintf( stderr, "ERROR: cannot open camera \n" );
    return -1;
  } 
  stream.set(CV_CAP_PROP_BUFFERSIZE, 3);

  //1 job for each channel
  jobData job[3] = {pixels, pixels, pixels};

  for(uint i=0; i<3; i++) {
    *job[i].moduleId = moduleIds[conv2D_5x5_Module];
    job[i].tag = i;


    respImg[i] = Mat::zeros(sizeWithBorder, sizeWithBorder, CV_32FC1);
  }
  
  for(uint i=0; i<fpgaCount; i++) {
    fpgas[i].setSuccessCb(&showJobResp);
    //fpgas[i].setFailedCb(&showJobResp);
  }
  Mat image;

  while(1) {
    
    stream.read(image);
    if(image.empty()) {
      break;
    }
    resize(image, image, cv::Size(width, width), 0, 0, CV_INTER_LINEAR);
    flip(image,image,1);

    copyMakeBorder(image, image, border, border, border, border, BORDER_REPLICATE);
    
    for(int i = 0; i < image.rows; i++) {
      for(int j = 0; j < image.cols; j++) {
        Vec3b bgrPixel = image.at<Vec3b>(i, j);
        job[0].payload[i*sizeWithBorder + j] = bgrPixel[0];
        job[1].payload[i*sizeWithBorder + j] = bgrPixel[1];
        job[2].payload[i*sizeWithBorder + j] = bgrPixel[2];
      }
    }
    sendJob(&job[0], pixels);
    sendJob(&job[1], pixels);
    sendJob(&job[2], pixels);
    srcLock.lock();
    resize(image, srcImg, cv::Size(sizeWithBorder*3, sizeWithBorder*3), 0, 0, CV_INTER_AREA);
    srcLock.unlock();
  }
  void *status;
  //run = 0;
  pthread_join(tShow, &status);

  connection_close();
  return 0;
}
