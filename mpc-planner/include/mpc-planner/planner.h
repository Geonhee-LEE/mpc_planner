#ifndef MPC_PLANNER_H
#define MPC_PLANNER_H

#include <mpc-planner-types/data_types.h>

#include <ros_planner_utils/profiling.h>

#include <memory>
#include <vector>

namespace MPCPlanner
{
    class RealTimeData;
    class State;
    class ControllerModule;
    class Solver;

    struct PlannerOutput
    {
        Trajectory trajectory;
        bool success{false};

        PlannerOutput(double dt, int N) : trajectory(dt, N) {}

        PlannerOutput() = default;
    };

    class Planner
    {
    public:
        Planner();

    public:
        PlannerOutput solveMPC(State &state, const RealTimeData &data);
        double getSolution(int k, std::string &&var_name);

        void onDataReceived(RealTimeData &data, std::string &&data_name);

        void visualize(const State &state, const RealTimeData &data);

        void reset(State &state, RealTimeData &data);

        bool isObjectiveReached(const RealTimeData &data) const;

    private:
        std::shared_ptr<Solver> _solver;
        PlannerOutput _output;

        std::vector<std::shared_ptr<ControllerModule>> _modules;

        std::unique_ptr<RosTools::Benchmarker> _benchmarker;
    };

}

#endif