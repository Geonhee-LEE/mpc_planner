import casadi as cd
import numpy as np

from util.files import model_map_path, write_to_yaml

# Returns discretized dynamics of a given model (see below)
def discrete_dynamics(z, model, integrator_stepsize):
    import forcespro.nlp

    """
    @param z: state vector (u, x)
    @param model: Model of the system
    @param integrator_stepsize: Integrator stepsize in seconds
    @return:
    """
    # We use an explicit RK4 integrator here to discretize continuous dynamics

    return forcespro.nlp.integrate(
        model.continuous_model,
        z[model.nu:model.nu+model.nx],
        z[0:model.nu],
        integrator=forcespro.nlp.integrators.RK4,
        stepsize=integrator_stepsize)


def numpy_to_casadi(x: np.array) -> cd.SX:
    result = None
    for param in x:
        if result is None:
            result = param
        else:
            result = cd.vertcat(result,param)
    return result

class DynamicsModel:

    def __init__(self):
        self.nu = 0  # number of control variables
        self.nx = 0  # number of states

        self.states = []
        self.inputs = []

        self.lower_bound = []
        self.upper_bound = []

    def get_nvar(self):
        return self.nu + self.nx

    def acados_symbolics(self):
        x = cd.SX.sym('x', self.nx)  # [px, py, vx, vy]
        u = cd.SX.sym('u', self.nu)  # [ax, ay]
        z = cd.vertcat(u, x)
        self.load(z)
        return z

    def get_acados_dynamics(self):
        self._x_dot = cd.SX.sym('x_dot', self.nx)

        f_expl = numpy_to_casadi(self.continuous_model(self._z[self.nu:], self._z[:self.nu]))
        f_impl = self._x_dot - f_expl
        return f_expl, f_impl

    def get_acados_x(self):
        return self._z[self.nu:]

    def get_acados_x_dot(self):
        return self._x_dot

    def get_acados_u(self):
        return self._z[:self.nu]

    def load(self, z):
        self._z = z

    def save_map(self):
        file_path = model_map_path()

        map = dict()
        for idx, state in enumerate(self.states):
            map[state] = ["x", idx + self.nu]

        for idx, input in enumerate(self.inputs):
            map[input] = ["u", idx]

        write_to_yaml(file_path, map)

    def get(self, state_or_input):
        if state_or_input in self.states:
            i = self.states.index(state_or_input)
            return self._z[self.nu + i]
        elif state_or_input in self.inputs:
            i = self.inputs.index(state_or_input)
            return self._z[i]
        else:
            raise IOError(f"Requested a state or input `{state_or_input}' that was neither a state nor an input for the selected model")

    def get_bounds(self, state_or_input):
        if state_or_input in self.states:
            i = self.states.index(state_or_input)
            return self.lower_bound[self.nu + i], self.upper_bound[self.nu + i],  self.upper_bound[self.nu + i] - self.lower_bound[self.nu + i]
        elif state_or_input in self.inputs:
            i = self.inputs.index(state_or_input)
            return self.lower_bound[i], self.upper_bound[i], self.upper_bound[i] - self.lower_bound[i]
        else:
            raise IOError(f"Requested a state or input `{state_or_input}' that was neither a state nor an input for the selected model")

class SecondOrderUnicycleModel(DynamicsModel):

    def __init__(self):
        super().__init__()
        self.nu = 2  # number of control variables
        self.nx = 4  # number of states

        self.states = ['x', 'y', 'psi', 'v']
        self.inputs = ['a', 'w']

        self.lower_bound = [-2., -2., -200., -200., -np.pi, -2.0]
        self.upper_bound = [2., 2., 200., 200., np.pi, 3.0]

    def continuous_model(self, x, u):

        a = u[0]
        w = u[1]
        psi = x[2]
        v = x[3]

        return np.array([v * cd.cos(psi),
                         v * cd.sin(psi),
                         w,
                         a])

class ContouringSecondOrderUnicycleModel(DynamicsModel):

    def __init__(self):
        super().__init__()
        self.nu = 2  # number of control variables
        self.nx = 5  # number of states

        self.states = ['x', 'y', 'psi', 'v', 'spline']
        self.inputs = ['a', 'w']

        self.lower_bound = [-2., -2., -200., -200., -np.pi, -2.0, 0.0]
        self.upper_bound = [2., 2., 200., 200., np.pi, 3.0, 2000.0]

    def continuous_model(self, x, u):

        a = u[0]
        w = u[1]
        psi = x[2]
        v = x[3]

        return np.array([v * cd.cos(psi),
                         v * cd.sin(psi),
                         w,
                         a,
                         v])


class RealTimeModel(DynamicsModel):

    def __init__(self, settings):
        super(RealTimeModel, self).__init__()
