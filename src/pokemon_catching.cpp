#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <geometry_msgs/Twist.h>
#include <string>

using namespace std;
using namespace cv;

static const bool DEBUG = false;

class PokemonCatching
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  ros::Publisher vel_pub_;
  int flag = 1;
  int roi_tl_x;
  int roi_tl_y;
  int roi_br_x;
  int roi_br_y;
  int roi_height;
  int roi_width;
  int exit = 0;
  string folder_path = "/home/wzl/pokemon_ws/";


public:
  PokemonCatching()
    : it_(nh_)
  {
    // Subscribe modified video stream from pokemon_searching
    image_sub_ = it_.subscribe("/pokemon_go/searcher", 1, &PokemonCatching::imageCb, this);
    vel_pub_ = nh_.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 1);
  }

  ~PokemonCatching()
  {
    // cv::destroyWindow(OPENCV_WINDOW);
  }

  void detectRect(cv_bridge::CvImagePtr & cv_ptr){
    Mat threshold_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    Mat src_gray;
    Mat frame;
    //自定义形态学元素结构
    cv::Mat element5(9, 9, CV_8U, cv::Scalar(1));//5*5正方形，8位uchar型，全1结构元素  
    Scalar color = Scalar(0,255,0);
    cv::Mat closed;
    Rect rect;
    Rect rect1;

    frame = cv_ptr->image;
    Rect roiRect(roi_tl_x, roi_tl_y, roi_width, roi_height);
    Mat roi = frame(roiRect);
    Mat image;
    Mat image2;
    Mat image3;
    if(DEBUG){
		  image = roi.clone();
		  image2 = roi.clone();
		  image3 = roi.clone();
    }

    cvtColor(roi, src_gray, COLOR_BGR2GRAY);  //颜色转换
    Canny(src_gray, threshold_output, 70, 100, (3, 3));   //使用Canny检测边缘
    cv::morphologyEx(threshold_output, closed, cv::MORPH_CLOSE, element5);  //形态学闭运算函数  

    if(DEBUG){
      // imshow("canny", threshold_output);
      // waitKey();
      imshow("erode", closed);
      // waitKey();
    }

    // findContours(closed, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point());
    findContours(closed, contours, hierarchy, CV_RETR_CCOMP, CHAIN_APPROX_NONE, Point());
    // findContours(threshold_output, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE, Point());

    if(DEBUG){
      cout << "contours.size()" << contours.size() << endl;
    }
    
    rect = boundingRect(contours[0]);
    
    if(DEBUG){
      for (int i = 0; i < contours.size(); i++){
        for (int j = 0; j < contours[i].size(); j++){

          // cout << contours[i][j] << endl;
          image.at<Vec3b>(contours[i][j].y, contours[i][j].x) = (0, 0, 255);
        }

      }
      // show all borders
      // imshow("allborders", image);
      // waitKey();
      // show rect in 0th
      rectangle(image2,rect,color, 2);
      imshow("rectin0", image2);
      // waitKey();
      // show tect in 1th
      // rect1 = boundingRect(contours[1]);
      // rectangle(image3,rect1,color, 2);
      // imshow("rectin1", image3);
      
      waitKey(3);

    }
    
    // add(frame, image, frame);
    // cvSetImageROI(frame, currentRect);


    // move
    if((rect.tl().x > 0)&&(rect.tl().y >0)&&(rect.br().x < roi_width)&&(rect.br().y < roi_height)){
      ROS_INFO("MOVE");
      geometry_msgs::Twist speed;
      // // x前后线速度 z左右旋转角速度
      speed.linear.x=0.2;
      // 左边空隙大
      if(rect.tl().x - (roi_width-rect.br().x) > 3){
        if(DEBUG)
          cout << "in z = 1" << endl;
        speed.angular.z=-0.1;
      }
      // 右边空隙大
      else if(rect.tl().x - (roi_width-rect.br().x) < -3){
        if(DEBUG)
          cout << "in z = -1" << endl;
        speed.angular.z=0.1;
      }
      else{
        if(DEBUG)
          cout << "in z = 0" << endl;
        speed.angular.z=0;
      }
      vel_pub_.publish(speed); 
      ros::Duration(0.2).sleep(); 
    }
    // left width more then 5
    // else if(rect.tl().x>5){
    //   ROS_INFO("GO RIGHT");
    //   geometry_msgs::Twist speed;
    //   speed.linear.x=0.02;
    //   speed.angular.z=-0.1;
    //   vel_pub_.publish(speed); 
    //   ros::Duration(0.1).sleep(); 
    // }
    // // right width more then 5
    // else if((roi_width-rect.br().x)>5){
    //   ROS_INFO("GO LEFT");
    //   geometry_msgs::Twist speed;
    //   speed.linear.x=0.02;
    //   speed.angular.z=0.1;
    //   vel_pub_.publish(speed); 
    //   ros::Duration(0.1).sleep(); 
    // }
    // stop
    else{
      ROS_INFO("STOP");
      vel_pub_.publish(geometry_msgs::Twist());
      exit = 1;
      rect1 = boundingRect(contours[1]);
      rectangle(roi,rect,color, 2);
      rectangle(frame, cv::Point(roi_tl_x, roi_tl_y), cv::Point(roi_br_x, roi_br_y), CV_RGB(255,0,0));
      // save img
      vector<cv::String> file_names;
      vector<string> split_string;
      glob(folder_path, file_names);
      int pokemon_img_num = 0;
      for (int i = 0; i < file_names.size(); i++) {
        // ROS_INFO("filename: %s", file_names[i].c_str());
        int pos = file_names[i].find_last_of("/");
        string file_name(file_names[i].substr(pos + 1));
        if(file_name.compare(0, 7, "pokemon")==0){
          pokemon_img_num++;

        }
      }
      pokemon_img_num ++;
      string file_name = folder_path + "pokemon_" + to_string(pokemon_img_num) + ".jpg";
      imwrite(file_name, frame);
      ros::shutdown();
      return;
    }
    // exit = 1;
    // image_sub_.shutdown();
  }

  void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
    // ROS_INFO("redy to detect ");
    if(flag==1){
      int height = cv_ptr->image.rows;
      int width = cv_ptr->image.cols;
      roi_tl_x = width/3;
      roi_tl_y = height/16;
      roi_br_x = 2*width/3;
      roi_br_y = 5*height/8;
      roi_height = roi_br_y-roi_tl_y;
      roi_width = roi_br_x-roi_tl_x;
      flag = 5;

    }
    if(exit!=1)
      detectRect(cv_ptr);
    // cout << "finish" << endl;
    // 1*1 半格
    if(flag == 2){
      ROS_INFO("detect finish");
      // ros::Rate loopRate(10);
      geometry_msgs::Twist speed;
      // // x前后线速度 z左右旋转角速度
      speed.linear.x=1;
      // // speed.angular.z=0/30; 
      vel_pub_.publish(speed); 
      ros::Duration(1).sleep(); 
      vel_pub_.publish(geometry_msgs::Twist()); 
      flag = 0;
    }

  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "pokemon_catching");
  PokemonCatching pc;
  ros::spin();
  return 0;
}