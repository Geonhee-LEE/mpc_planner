#include "mpc-planner-modules/gaussian_constraints.h"

#include <mpc-planner-util/parameters.h>

#include <ros_planner_utils/visuals.h>
#include <ros_planner_utils/math.hpp>

#include <algorithm>

namespace MPCPlanner
{
  GaussianConstraints::GaussianConstraints(std::shared_ptr<Solver> solver)
      : ControllerModule(ModuleType::CONSTRAINT, solver, "gaussian_constraints")
  {
    LOG_INFO("Initializing GaussianConstraints Module");
  }

  void GaussianConstraints::update(State &state, const RealTimeData &data)
  {
    (void)state;
    (void)data;
  }

  void GaussianConstraints::setParameters(const RealTimeData &data, int k)
  {

    _solver->setParameter(k, "ego_disc_radius", CONFIG["robot_radius"].as<double>());
    for (int d = 0; d < CONFIG["n_discs"].as<int>(); d++)
      _solver->setParameter(k, "ego_disc_" + std::to_string(d) + "_offset", 0.); /** @todo Fix offsets! */

    std::vector<DynamicObstacle> copied_obstacles = data.dynamic_obstacles;

    for (size_t i = 0; i < copied_obstacles.size(); i++)
    {
      const auto &obstacle = copied_obstacles[i];

      if (obstacle.prediction.type == PredictionType::GAUSSIAN)
      {
        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_x", obstacle.prediction.steps[k - 1].position(0));
        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_y", obstacle.prediction.steps[k - 1].position(1));

        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_minor", obstacle.prediction.steps[k - 1].minor_radius);
        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_major", obstacle.prediction.steps[k - 1].major_radius);

        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_risk", CONFIG["probabilistic"]["risk"].as<double>());
        _solver->setParameter(k, "gaussian_obst_" + std::to_string(i) + "_r", CONFIG["obstacle_radius"].as<double>());
      }
    }
  }

  bool GaussianConstraints::isDataReady(const RealTimeData &data, std::string &missing_data)
  {
    if (data.dynamic_obstacles.size() != CONFIG["max_obstacles"].as<unsigned int>())
    {
      missing_data += "Obstacles ";
      return false;
    }

    for (size_t i = 0; i < data.dynamic_obstacles.size(); i++)
    {
      if (data.dynamic_obstacles[i].prediction.steps.empty())
      {
        missing_data += "Obstacle Prediction ";
        return false;
      }

      if (data.dynamic_obstacles[i].prediction.type != PredictionType::GAUSSIAN)
      {
        missing_data += "Obstacle Prediction (Type is not Gaussian) ";
        return false;
      }
    }

    return true;
  }

  void GaussianConstraints::visualize(const RealTimeData &data)
  {
    LOG_DEBUG("GaussianConstraints::visualize");
    auto &publisher = VISUALS.getPublisher(_name);

    auto &ellipsoid = publisher.getNewPointMarker("CYLINDER");

    for (auto &obstacle : data.dynamic_obstacles)
    {

      for (int k = 1; k < _solver->N; k += CONFIG["visualization"]["draw_every"].as<int>())
      {
        ellipsoid.setColorInt(k, _solver->N, 0.5);

        double chi = RosTools::ExponentialQuantile(0.5, 1.0 - CONFIG["probabilistic"]["risk"].as<double>());
        ellipsoid.setScale(2 * (obstacle.prediction.steps[k - 1].major_radius * std::sqrt(chi) + obstacle.radius),
                           2 * (obstacle.prediction.steps[k - 1].major_radius * std::sqrt(chi) + obstacle.radius), 0.005);

        ellipsoid.addPointMarker(obstacle.prediction.steps[k - 1].position);
      }
    }
    publisher.publish();
  }

} // namespace MPCPlanner
