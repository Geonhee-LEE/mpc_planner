#include <mpc-planner/planner.h>

/** @note: Autogenerated */
#include <mpc-planner-modules/modules.h>

#include <mpc_planner_types/realtime_data.h>
#include <mpc-planner-solver/solver_interface.h>

#include <mpc-planner-util/load_yaml.hpp>
#include <mpc-planner-util/parameters.h>
#include <mpc-planner-util/data_visualization.h>

#include <ros_tools/logging.h>
#include <ros_tools/visuals.h>

namespace MPCPlanner
{

    Planner::Planner()
    {
        // Initialize the solver
        _solver = std::make_shared<Solver>();
        _solver->reset();

        initializeModules(_modules, _solver);

        _benchmarker = std::make_unique<RosTools::Benchmarker>("optimization");
    }

    // Given real-time data, solve the MPC problem
    PlannerOutput Planner::solveMPC(State &state, RealTimeData &data)
    {
        LOG_MARK("Planner::solveMPC");
        bool was_feasible = _output.success;
        _output = PlannerOutput(_solver->dt, _solver->N);

        _module_data = ModuleData(); // Reset module data

        // Check if all modules have enough data
        bool is_data_ready = true;
        std::string missing_data;
        for (auto &module : _modules)
            is_data_ready = is_data_ready & module->isDataReady(data, missing_data);

        if (!is_data_ready)
        {
            LOG_WARN_THROTTLE(3000, "Data is not ready, missing " + missing_data + "\b");
            _output.success = false;
            return _output;
        }
        LOG_MARK("Data checked");

        // Set the initial guess
        if (was_feasible)
            _solver->initializeWarmstart(state, CONFIG["shift_previous_solution_forward"].as<bool>());
        else
        {
            _solver->initializeWithState(state);
        }
        // Set the initial state
        _solver->setXinit(state);

        LOG_MARK("Updating modules");

        // Update all modules
        {
            PROFILE_SCOPE("Update");

            for (auto &module : _modules)
                module->update(state, data, _module_data);
        }

        LOG_MARK("Setting parameters");
        {
            PROFILE_SCOPE("SetParameters");
            for (int k = 0; k < _solver->N; k++)
            {
                for (auto &module : _modules)
                {
                    if (k == 0 && module->type == ModuleType::CONSTRAINT)
                        continue;

                    module->setParameters(data, _module_data, k);
                }
            }
        }

        _solver->loadWarmstart();

        // Solve MPC
        LOG_MARK("Solve optimization");
        int exit_flag;
        {
            PROFILE_SCOPE("Optimization");
            _benchmarker->start();
            exit_flag = EXIT_CODE_NOT_OPTIMIZED_YET;
            for (auto &module : _modules)
            {
                exit_flag = module->optimize(state, data, _module_data);
                if (exit_flag != EXIT_CODE_NOT_OPTIMIZED_YET)
                    break;
            }
            if (exit_flag == EXIT_CODE_NOT_OPTIMIZED_YET)
                exit_flag = _solver->solve();
            _benchmarker->stop();
        }

        if (exit_flag != 1)
        {
            _output.success = false;
            LOG_WARN("MPC did not find a solution");
            LOG_VALUE("Exit Flag", exit_flag); /** @todo: Convertion to text */
            return _output;
        }

        _output.success = true;
        for (int k = 1; k < _solver->N; k++)
            _output.trajectory.add(_solver->getOutput(k, "x"), _solver->getOutput(k, "y"));

        LOG_MARK("Planner::solveMPC done");

        return _output;
    }

    double Planner::getSolution(int k, std::string &&var_name)
    {
        return _solver->getOutput(k, std::forward<std::string>(var_name));
    }

    void Planner::onDataReceived(RealTimeData &data, std::string &&data_name)
    {
        for (auto &module : _modules)
            module->onDataReceived(data, std::forward<std::string>(data_name));
    }

    void Planner::visualize(const State &state, const RealTimeData &data)
    {
        PROFILE_FUNCTION();
        LOG_MARK("Planner::visualize");
        (void)state;

        for (auto &module : _modules)
            module->visualize(data, _module_data);

        visualizeTrajectory(_output.trajectory, "planned_trajectory", true, 0.2);

        visualizeObstacles(data.dynamic_obstacles, "obstacles", true);
        visualizeObstaclePredictions(data.dynamic_obstacles, "obstacle_predictions", true);
        visualizeRobotArea(state.getPos(), state.get("psi"), data.robot_area, "robot_area", true);

        std::vector<double> angles;
        for (int k = 1; k < _solver->N; k++)
            angles.emplace_back(_solver->getOutput(k, "psi"));

        visualizeRectangularRobotArea(state.getPos(), state.get("psi"),
                                      CONFIG["robot"]["length"].as<double>(), CONFIG["robot"]["width"].as<double>(),
                                      "robot_rect_area", true);

        visualizeRobotAreaTrajectory(_output.trajectory, angles, data.robot_area, "robot_area_trajectory", true, 0.1);
        LOG_MARK("Planner::visualize Done");
    }

    void Planner::reset(State &state, RealTimeData &data)
    {
        _solver->reset(); // Reset the solver

        for (auto &module : _modules) // Reset modules
            module->reset();

        state = State(); // Reset the state
        data.reset();    // Reset the data
    }

    bool Planner::isObjectiveReached(const RealTimeData &data) const
    {
        bool objective_reached = true;
        for (auto &module : _modules)
            objective_reached = objective_reached && module->isObjectiveReached(data);
        return objective_reached;
    }
}