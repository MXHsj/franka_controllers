// Copyright (c) 2017 Franka Emika GmbH
// Use of this source code is governed by the Apache-2.0 license, see LICENSE
#include <franka_controllers/joint_position_controller.h>

#include <cmath>

#include <controller_interface/controller_base.h>
#include <hardware_interface/hardware_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <pluginlib/class_list_macros.h>
#include <ros/ros.h>

/*
  - used to bring the robot to home position
  - author: Xihan Ma
  - last modified date: 12/23/2020
*/

#define deg2rad M_PI / 180
namespace franka_controller
{

  bool JointPositionController::init(hardware_interface::RobotHW *robot_hardware, ros::NodeHandle &node_handle)
  {
    position_joint_interface_ = robot_hardware->get<hardware_interface::PositionJointInterface>();
    if (position_joint_interface_ == nullptr)
    {
      ROS_ERROR("JointPositionExampleController: Error getting position joint interface from hardware!");
      return false;
    }
    std::vector<std::string> joint_names;
    if (!node_handle.getParam("joint_names", joint_names))
    {
      ROS_ERROR("JointPositionExampleController: Could not parse joint names");
    }
    if (joint_names.size() != 7)
    {
      ROS_ERROR_STREAM("JointPositionExampleController: Wrong number of joint names, got "
                       << joint_names.size() << " instead of 7 names!");
      return false;
    }
    position_joint_handles_.resize(7);
    for (size_t i = 0; i < 7; ++i)
    {
      try
      {
        position_joint_handles_[i] = position_joint_interface_->getHandle(joint_names[i]);
      }
      catch (const hardware_interface::HardwareInterfaceException &e)
      {
        ROS_ERROR_STREAM(
            "JointPositionExampleController: Exception getting joint handles: " << e.what());
        return false;
      }
    }
    // std::array<double, 7> q_start{{0, -M_PI_4, 0, -3 * M_PI_4, 0, M_PI_2, M_PI_4}};
    // for (size_t i = 0; i < q_start.size(); i++) {
    //   if (std::abs(position_joint_handles_[i].getPosition() - q_start[i]) > 0.1) {
    //     ROS_ERROR_STREAM(
    //         "JointPositionExampleController: Robot is not in the expected starting position for "
    //         "running this example. Run `roslaunch franka_example_controllers move_to_start.launch "
    //         "robot_ip:=<robot-ip> load_gripper:=<has-attached-gripper>` first.");
    //     return false;
    //   }
    // }

    return true;
  }

  double kp = 0.0005;
  double max_increment = 0.00015;
  double stop_threshold = 0.0005;
  std::array<double, 7> q_start{{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};

  void JointPositionController::starting(const ros::Time & /* time */)
  {
    for (size_t i = 0; i < 7; ++i)
    {
      initial_pose_[i] = position_joint_handles_[i].getPosition();
    }
    elapsed_time_ = ros::Duration(0.0);
    int home_id = 2;
    switch (home_id)
    {
    case 0:
      q_start = {{0.0, -1.0, 0.1, -2.5, 0.1, 1.5, 1.57}}; // flange x -> base -y US
      // q_start = {{0.0, -1.0, 0.1, -2.5, 0.1, 1.5, 0.0}};  // flange x -> base +x US
      break;
    case 1:
      q_start = {{0.0, -0.373, 0.0, -2.025, 0.0, 1.635, 0.523}}; // oct eef
      break;
    case 2:
      q_start = {
          {0.0, -30 * deg2rad, 0.0, -120 * deg2rad, 0.0, M_PI_2, -60 * deg2rad}}; // bedside US
    }
  }

  void JointPositionController::update(const ros::Time & /*time*/, const ros::Duration &period)
  {
    // elapsed_time_ += period;
    // double delta_angle = M_PI / 16 * (1 - std::cos(M_PI / 5.0 * elapsed_time_.toSec())) * 0.2;

    for (size_t i = 0; i < 7; ++i)
    {
      double p_error = q_start[i] - position_joint_handles_[i].getPosition();
      if (std::abs(p_error) > stop_threshold)
      {
        // angular error for each joint
        double pos_increment = p_error * kp;
        // limit max joint angle increment
        if (std::abs(pos_increment) > max_increment)
        {
          pos_increment = max_increment * (pos_increment / std::abs(pos_increment));
        }
        // proportional control
        position_joint_handles_[i].setCommand(position_joint_handles_[i].getPosition() + pos_increment);
      }
      else
      {
        // ROS_INFO("joint%d ready", i);
        std::cout << "joint" << i << " ready;";
      }
    }
    std::cout << std::endl;
  }

} // namespace franka_controllers

PLUGINLIB_EXPORT_CLASS(franka_controller::JointPositionController,
                       controller_interface::ControllerBase)
