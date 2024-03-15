#ifndef __ROS1_DINGO_PLANNER_H__
#define __ROS1_DINGO_PLANNER_H__

#include <mpc_planner/planner.h>

#include <mpc_planner_solver/solver_interface.h>

#include <mpc_planner_types/realtime_data.h>

#include <mpc_planner_msgs/obstacle_array.h> /** @Todo: Replace! */

#include <ros_tools/helpers.h>

#include <ros/ros.h>

#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>

#include <std_srvs/Empty.h>
#include <robot_localization/SetPose.h>

#include <memory>

using namespace MPCPlanner;

class dingoPlanner
{
public:
    dingoPlanner(ros::NodeHandle &nh);

    void initializeSubscribersAndPublishers(ros::NodeHandle &nh);

    void startEnvironment();

    void loop(const ros::TimerEvent &event);

    void stateCallback(const nav_msgs::Odometry::ConstPtr &msg);
    void statePoseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg); /** @note: Connects to the dingoSimulator */
    void goalCallback(const geometry_msgs::PoseStamped::ConstPtr &msg);
    void pathCallback(const nav_msgs::Path::ConstPtr &msg);
    void obstacleCallback(const mpc_planner_msgs::obstacle_array::ConstPtr &msg);

    void reset();

private:
    std::unique_ptr<Planner> _planner;

    RealTimeData _data;
    State _state;

    ros::Timer _timer;

    std::unique_ptr<RosTools::Benchmarker> _benchmarker;

    // Subscribers and publishers
    ros::Subscriber _state_sub;
    ros::Subscriber _goal_sub;
    ros::Subscriber _path_sub;
    ros::Subscriber _obstacle_sub;

    ros::Publisher _cmd_pub;
    ros::Publisher _ped_horizon_pub, _ped_integrator_step_pub, _ped_clock_frequency_pub;
    ros::ServiceClient _ped_start_client;

    std_srvs::Empty _reset_msg;
    robot_localization::SetPose _reset_pose_msg;
    ros::Publisher _reset_simulation_pub;
    ros::ServiceClient _reset_simulation_client;
    ros::ServiceClient _reset_ekf_client;

    bool isPathTheSame(const nav_msgs::Path::ConstPtr &path);

    void visualize();
};

#endif // __ROS1_dingo_PLANNER_H__