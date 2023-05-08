#include "LinearProgram.h"
#include <Eigen/Core>
#include "Matrix.h"

template<typename T>
using Matrix = MatrixUtil::Matrix<T>;

/// <summary>
/// Build objective from integral, start and end weights
/// </summary>
/// <param name="phi0"></param>
/// <param name="phi1"></param>
/// <param name="phi2"></param>
/// <param name="y"></param>
/// <returns></returns>
//inline IloNumExprArg buildObjective(Matrix<IloNum> phi0, RowVec<IloNum> phi1, RowVec<IloNum> phi2, Matrix<IloNumVar> y)
//{
//    IloNumExprArg obj = MatrixUtil::mulSum(phi0, y);
//    return obj;
//}

/// <summary>
/// 
/// </summary>
/// <param name="model"></param>
void RKDiscretize(IloModel& model, Matrix<IloNum> Fc, Matrix<IloNum> Fy, Matrix<IloNum> Fu, Matrix<IloNumVar> y, Matrix<IloNumVar> u, double dt, double t0 = 0)
{
    const size_t n = y.rows();
    const size_t m = y.cols();

    for (auto j = 0; j < m - 1; j++) {
        auto t = j * dt + t0;
        for (auto i = 0; i < n; i++) {
            /*
            auto _c = Fc(0, j);
            */
            auto _y = MatrixUtil::dot(Fy.row(j), y.col(j));
            auto _u = MatrixUtil::dot(Fu.row(j), u.col(j));

            model.add(y(i, j + 1) == y(i, j) + dt * (_y + _u));
        }
    }
}

// TODO: u_steps, y_steps optional
bool LinearProgram::solve(double t0, double t1, size_t steps, size_t dim)
{
    double dt = ((t1 - t0) / steps);

    IloEnv env;
    IloModel model(env);

    // Populate matrices
    Matrix<IloNumVar> u(dim, steps);
    Matrix<IloNumVar> y(dim, steps);

    for (auto j = 0; j < steps; j++) {   // Col
        for (auto i = 0; i < dim; i++) { // Row
            u(i, j) = IloNumVar(env, -1.0, 1.0);
            y(i, j) = IloNumVar(env, DBL_MIN, DBL_MAX);
        }
    }

    // Discretize
    Matrix<IloNum> F[3] = {
        Matrix<IloNum>::Constant(steps, dim, 1.0),      // F_c(t)
        Matrix<IloNum>::Constant(steps, dim, 0.0),      // F_y(t)
        Matrix<IloNum>::Constant(steps, dim, -1.0),     // F_u(t)
    };

    RKDiscretize(model, F[0], F[1], F[2], y, u, dt, t0);

    // Build objective function
    // TODO: Pass as (optional) parameter
    Matrix<IloNum> phi[3] = {
        Matrix<IloNum>::Constant(steps, dim, 1.0),  // integral of y(t)
        Matrix<IloNum>::Constant(steps, 1, 0.0),    // y(t0)
        Matrix<IloNum>::Constant(steps, 1, 0.0),    // y(t1)
    };

    // buildObjective(phi[0], phi[1], phi[2], y)
    IloNumExprArg obj = MatrixUtil::mulSum(phi[0], y);
    model.add(IloMinimize(env, obj));

    // Add boundary conditions
    // TODO: Function
    model.add(y(0, 0) == 1);

    try {
        // Solve
        IloCplex cplex(model);
        cplex.solve();

        // Output
        dynamics = std::vector<double>(steps);
        control = std::vector<double>(steps);
        for (auto j = 0; j < steps; j++) {
            control[j] = cplex.getValue(u(0, j));
            dynamics[j] = cplex.getValue(y(0, j));
        }

        return true;
    }
    catch (IloException& e) {
        std::cerr << "Concert exception caught: " << e << std::endl;
    }
    catch (...) {
        std::cerr << "An unknown error occured.";
    }

    return false;
}