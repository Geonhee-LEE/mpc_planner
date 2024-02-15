
#include <mpc-planner-modules/goal_module.h>

#include <mpc-planner-util/visuals.h>
#include <mpc-planner-util/parameters.h>

namespace MPCPlanner
{

    GoalModule::GoalModule(std::shared_ptr<Solver> solver)
        : ControllerModule(solver, ModuleType::OBJECTIVE, "goal_module")
    {
    }

    void GoalModule::update(const RealTimeData &data)
    {
    }

    void GoalModule::setParameters(const RealTimeData &data, int k)
    {
        if (k == 0)
            LOG_DEBUG("Goal Module::setParameters()");

        // Set the parameters for the solver
        _solver->setParameter(k, "goal_x", data.goal(0));
        _solver->setParameter(k, "goal_y", data.goal(1));

        _solver->setParameter(k, "goal_weight", CONFIG["weights"]["goal"].as<double>());
    }

    bool GoalModule::isDataReady(const RealTimeData &data, std::string &missing_data)
    {
        if (!data.goal_received)
            missing_data += "Goal ";

        return data.goal_received;
    }

    void GoalModule::visualize(const RealTimeData &data)
    {
        if (!data.goal_received)
            return;

        LOG_DEBUG("GoalModule::visualize()");
        auto &publisher = VISUALS.getPublisher(_name);
        auto &sphere = publisher.getNewPointMarker("SPHERE");

        sphere.setColorInt(5);
        sphere.setScale(0.4, 0.4, 0.4);
        sphere.addPointMarker(data.goal, 0.0);

        publisher.publish();
    }
};