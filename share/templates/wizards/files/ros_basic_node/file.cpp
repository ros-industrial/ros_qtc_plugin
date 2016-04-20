#include <ros/ros.h>

int main(int argc, char **argv)
{
  // Set up ROS.
  ros::init(argc, argv, "%{BaseName}");

  ROS_INFO("Hello world!");
}
