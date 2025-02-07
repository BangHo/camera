#include "camera/by_id_parking_func.cpp"
#include <chrono>
#include <thread>
#include <string>
// const std::string LEFT_CAM = "/home/bongbongbongbong/video/morningLeft.mp4";
// const std::string RIGHT_CAM = "/home/bongbongbongbong/video/morningRight.mp4";

const std::string LEFT_CAM = "/dev/v4l/by-id/usb-046d_Logitech_Webcam_C930e_E5BC8E9E-video-index0";
const std::string RIGHT_CAM = "/dev/v4l/by-id/usb-046d_Logitech_Webcam_C930e_5DCC8E9E-video-index0";


// 아직 변수 정리 중
int array_count = 0;
int boom_count = 0;
int mission_flag = 0;
bool lidar_stop = false;

cv::Mat img_color;
cv::Mat img_color_2;
int H, S, V;

bool finish_park = false;
bool impulse = false;

double left_array[6] = {0,0,0,0,0,0};
double right_array[6] = {0,0,0,0,0,0};
double array[6] = {0,0,0,0,0,0};
double pub_array[6] = {0,0,0,0,0,0};
double default_array[6] = {2.4, -2.2, 3.65, -4.2, 4.9, -6.2}; // k-city에 맞춘 이상적인 주차구역 좌표
float sum_array[6] ={0,0,0,0,0,0};
cv::Point ptOld1;

// void Parking::on_mouse(int event, int x, int y, int flags, void* userdata) 
// {
    // if (event == cv::EVENT_MOUSEMOVE) {
        // cv::Mat* image = static_cast<cv::Mat*>(userdata);
// 
        // 마우스 위치 좌표를 이미지에 표시
        // cv::putText(*image, "(" + std::to_string(x) + ", " + std::to_string(y) + ")",
                    // cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
        // cv::imshow("Image", *image);
    // }
// }

Parking::Parking()
: Node("parking_node")
{
 	lidar_flag_ = this->create_subscription<std_msgs::msg::Bool>(
      "/LiDAR/stop", rclcpp::SensorDataQoS(),
      [this](const std_msgs::msg::Bool::SharedPtr msg) { lidar_callback(msg); });

    mission_flag_ = this->create_subscription<std_msgs::msg::Int16>(
      "/Planning/mission", rclcpp::SensorDataQoS(),
      [this](const std_msgs::msg::Int16::SharedPtr msg) { mission_callback(msg); });

    center_xz_pub_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
      "/Vision/parking_points",
      rclcpp::SensorDataQoS());	cap_left_ = cv::VideoCapture(LEFT_CAM);
	cap_right_ = cv::VideoCapture(RIGHT_CAM);

	cap_left_.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap_left_.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    cap_right_.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap_right_.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    if (!cap_left_.isOpened())
    {
    	RCLCPP_ERROR(this->get_logger(), "왼쪽 안 켜짐!");
    	return;
    }

    if (!cap_right_.isOpened())
    {
    	RCLCPP_ERROR(this->get_logger(), "오른쪽 안 켜짐!");
    	return;
    }
	
    timer_ = this->create_wall_timer(std::chrono::milliseconds(1), std::bind(&Parking::image_processing, this));
}

/**
 * @brief 라이다로부터 플래그를 받음 lidar_stop
 * lidar_stop = true 인 경우 level2로 넘어감
 * @param msg
 */
void Parking::lidar_callback(std_msgs::msg::Bool::SharedPtr msg)
{
	lidar_stop = msg->data;
}

/**
 * @brief 플래닝으로부터 미션 번호를 받음
 * 10 = 뭐야
 * 6 = 또 뭐야
 * @param msg
 */
void Parking::mission_callback(std_msgs::msg::Int16::SharedPtr msg)
{
	mission_flag = msg->data;
	//RCLCPP_INFO(this->get_logger(), "sub message: '%s'", std::to_string(msg->data).c_str());
}

void Parking::image_processing()
{
	cv::Mat left_frame;
	cv::Mat right_frame;
	cv::Mat leftMask;
	cv::Mat rightMask;
	cv::Point leftCircle, rightCircle; // using yellow ball
	cv::Point2d top_XZ, mid_XZ, bottom_XZ; // using parking

	//만약에  cap에 이미지가 없으면 pass

	cap_left_ >> left_frame;
	cap_right_ >> right_frame;

	//double left_array[6] = {0,0,0,0,0,0};
	//leftMask = find_edge(left_frame, LEFT_CAM);
	//imshow("leftMask", leftMask);
	//cv::waitKey(1);
	//double *ptr_left = Parking::find_center(leftMask,left_frame, left_array, LEFT_CAM);

	

	//cv::resize(left_frame, left_frame, cv::Size(1280, 720));
	//cv::resize(right_frame, right_frame, cv::Size(1280, 720));

	// =================[ using stereo setting ]=========================
	//Parking::line_symmetry(left_frame, LEFT_CAM);
	//Parking::line_symmetry(right_frame, RIGHT_CAM);
	//cv::rectangle(left_frame, cv::Rect(0, 0, 1280, 100), cv::Scalar(0, 0, 0), -1); //상단 for koreatech
	//cv::rectangle(right_frame, cv::Rect(0, 0, 1280, 100), cv::Scalar(0, 0, 0), -1); //상단 for koreatech
	// rectangle(left_frame, Rect(0, 400, 1280, 720), Scalar(0, 0, 0), -1); //for k-citys
	// rectangle(right_frame, Rect(0, 0, 1280, 200), Scalar(0, 0, 0), -1); //for k-citys

	//imshow("right_frame", right_frame);
	//cv::waitKey(1);
	//imshow("left_frame", left_frame);
	//cv::waitKey(1);

	// ===================================================================

/////	leftMask = Parking::add_hsv_filter(left_frame, LEFT_CAM);
/////	rightMask = Parking::add_hsv_filter(right_frame, RIGHT_CAM);
/////	cv::Point2f ball_XZ;
/////	leftCircle = Parking::find_ball(leftMask, leftMask);
/////	rightCircle = Parking::find_ball(rightMask, rightMask);
/////	ball_XZ = Parking::find_xz(leftCircle, rightCircle, left_frame, right_frame, Parking::alpha, Parking::beta
/////	);
/////	cv::imshow("yellowLeft", leftMask);
/////	cv::waitKey(1);
	/////// cv::imshow("yellowRignt", rightMask);
	/////// cv::waitKey(1);
/////	cv::setMouseCallback("yellowLeft", on_mouse, &leftMask);
	/////// cv::setMouseCallback("yellowRignt", on_mouse, &rightMask);


	if((mission_flag == 10)||(mission_flag == 6))
	{
		if((lidar_stop == false) && (finish_park == false))
		{
			// cout << " Wating Lidar Stop" << endl;
			// imshow("Left Frame", left_frame);
			// imshow("Right Frame", right_frame);
		}
		else if((lidar_stop == true) && (finish_park == false))
		{	
			if(impulse == false)
			{
				// erp정지시 흔들림에 의해 생기는 오차 방지
				std::cout << "impulse !!" << std::endl;
				//ros::Duration(1.5).sleep(); // (구)1.5초 정지 코드
				std::chrono::milliseconds duration(2000);
				std::this_thread::sleep_for(duration);
				impulse = true;
			}

			leftMask = find_edge(left_frame, LEFT_CAM);
			rightMask = find_edge(right_frame, RIGHT_CAM);

			// imshow("Left_mask", leftMask);
			// cv::waitKey(1);
			// imshow("Right_mask", rightMask);
			// cv::waitKey(1);

			// cv::Mat test;
			// test = adapt_th(left_frame);
			// cv::imshow("TEST", test);
			// cv::waitKey(1);

			//std_msgs::msg::Float64MultiArray center_xz_msg;
			//double *ptr_left = Parking::find_center(leftMask, left_frame, left_array, LEFT_CAM);
			//double *ptr_right = Parking::find_center(rightMask, right_array, RIGHT_CAM);
			Parking::find_center(leftMask, left_array, LEFT_CAM); // array를 초기화 하는 위치가 여기 있어야하지 않나?
			Parking::find_center(rightMask, right_array, RIGHT_CAM);
			
			/*
			for(int i =0 ; i < 6; i++)
			{
				printf("배열 값[%d] : %lf\n",i, left_array[i]);
				//center_xz_msg.data.push_back(left_array[i]);
			}

			//center_xz_pub_->publish(center_xz_msg);
			*/
			

			if (left_array[0] && right_array[0])
			{
				bottom_XZ = Parking::find_xz({left_array[0],left_array[1]}, {right_array[0],right_array[1]}, left_frame, right_frame, Parking::alpha, Parking::beta);
				// mid_XZ = stereovision.`({left_array[2],left_array[3]}, {right_array[2],right_array[3]}, left_frame, right_frame, Parking::alpha, Parking::beta);
				top_XZ = Parking::find_xz({left_array[4],left_array[5]}, {right_array[4],right_array[5]}, left_frame, right_frame, Parking::alpha, Parking::beta);
				mid_XZ.x = (bottom_XZ.x + top_XZ.x)/2.0;
				mid_XZ.y = (bottom_XZ.y + top_XZ.y)/2.0;

				std::cout << "bottom_XZ : " << bottom_XZ << std::endl;
				std::cout << "mid_XZ : " << mid_XZ << std::endl;
				std::cout << "top_XZ : " << top_XZ << std::endl;

				array[0]= (bottom_XZ.x + Parking::gps_for_camera_z)/100.00;
				array[1]= -(bottom_XZ.y + Parking::gps_for_camera_x)/100.00;
				array[2]= (mid_XZ.x + Parking::gps_for_camera_z)/100.00;
				array[3]= -(mid_XZ.y + Parking::gps_for_camera_x)/100.00;
				array[4]= (top_XZ.x + Parking::gps_for_camera_z)/100.00;
				array[5]= -(top_XZ.y + Parking::gps_for_camera_x)/100.00;

				//############################################ 보험용 #################################################################################################
				if((abs(array[0]-default_array[0]) > 1.3) || (abs(array[1]-default_array[1]) > 1.3) || \
				(abs(array[2]-default_array[2]) > 1.3) || (abs(array[3]-default_array[3]) > 1.3) || \
				(abs(array[4]-default_array[4]) > 1.3) || (abs(array[5]-default_array[5]) > 1.3))
				{
		//////			// std::cout << "고정점 ++" << std::endl;
					boom_count++;

					for(int i=0; i < 6; i++)
					{
						array[i] = default_array[i];
					}
				}

				for(int i=0; i<6; i++)
				{
					sum_array[i] = sum_array[i] + array[i];
				}				

				// array_count++;
				std::cout << "array_count : " << array_count << std::endl;

				if(array_count == 20)
				{
					if(boom_count > 15)
					{
						std::cout << "주차실패 ㅠㅠㅠㅠ" << std::endl;
					}
					else
					{
						std::cout << " 봄 ? \n 이게 바로 비전 클라스 우리 잘못 아니니 뭐라 하려면 제어탓. \n ^~^" << std::endl;
					}
					
					std::cout << "!!!!!!!!!!!!!!!!!!" << std::endl;
					std_msgs::msg::Float64MultiArray center_xz_msg; 
					center_xz_msg.data.clear();

					for(int i=0; i<6; i++)
					{
						pub_array[i] = sum_array[i]/(double)array_count;
						center_xz_msg.data.push_back(pub_array[i]);
						printf("pub_array[%d] : %f\n", i, pub_array[i]);
					}
					
					center_xz_pub_->publish(center_xz_msg);
					finish_park = true;
					//std::cout << " Finish !!!! " << std::endl;
				}
			}
		}
		else if((lidar_stop == true) && (finish_park == true))
		{
			//std::cout << " Finish !!!! " << std::endl;
		}
	}
	else
	{
		imshow("right_frame(waiting misiion)", right_frame);
		cv::waitKey(1);
		imshow("left_frame(waiting misiion)", left_frame);
		cv::waitKey(1);
	}
}


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
	std::cout << "=======스테레오 준비완료=======" << std::endl;
    rclcpp::spin(std::make_shared<Parking>()); 
    rclcpp::shutdown();
    return 0;
}































/**
 * @brief 마우스 왼쪽 클릭을 하면 마우스가 있는 지점의 HSV 색상을 알려줌
 * @param event 
 * @param x 
 * @param y 
 * @param flags 
 * @param param 
 */
void mouse_callback(int event, int x, int y, int flags, void *param)
{
	if (event == cv::EVENT_LBUTTONDBLCLK)
	{
		cv::Vec3b color_pixel = img_color.at<cv::Vec3b>(y, x);

		cv::Mat hsv_color = cv::Mat(1, 1, CV_8UC3, color_pixel);


		H = hsv_color.at<cv::Vec3b>(0, 0)[0];
		S = hsv_color.at<cv::Vec3b>(0, 0)[1];
		V = hsv_color.at<cv::Vec3b>(0, 0)[2];

		std::cout << "left H= " << H << std::endl;
		std::cout << "left S= " << S << std::endl;
		std::cout << "left V = " << V << "\n" << std::endl;

	}
}

// float camera_values()
// {
//     cv::Mat K(3,3,CV_64F);

//     K = (Mat_<_Float64>(3, 3) << 538.39128993937,   0.,   308.049009633327, 0.,  539.777910345317,  248.065244763797, 0., 0., 1.);

//     cout << "K : " << K << endl;


//     cv::Size imageSize(640,480);
//     double apertureWidth = 0;
//     double apertureHeight = 0;
//     double fieldOfViewX;
//     double fieldOfViewY;
//     double focalLength2;
//     cv::Point2d principalPoint;
//     double aspectRatio;
//     cv::calibrationMatrixValues(K, imageSize, apertureWidth, apertureHeight, fieldOfViewX, fieldOfViewY, focalLength2, principalPoint, aspectRatio);

    
//     cout << fieldOfViewX << endl;

//     return (float)focalLength2;
// }

/**
 * @brief 마우스 왼쪽 버튼을 누르고 땔 때마다 그 위치의 좌표를 알려줌
 * 
 * @param event 
//  * @param x 
//  * @param y 
//  * @param flags 
//  */
// 
void Parking::on_mouse(int event, int x, int y, int flags, void *)
{
	switch (event)
	{
	case cv::EVENT_LBUTTONDOWN:
		ptOld1 = cv::Point(x, y);
		std::cout << "EVENT_LBUTTONDOWN: " << x << ", " << y << std::endl;
		break;
	case cv::EVENT_LBUTTONUP:
		std::cout << "EVENT_LBUTTONUP: " << x << ", " << y << std::endl;
		break;
	}
}
