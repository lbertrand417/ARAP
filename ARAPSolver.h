#pragma once
#include <igl/opengl/glfw/Viewer.h>
#include "ControlPoint.h"

using namespace Eigen;

void find_neighbors(const MatrixXd V, const MatrixXi F);
void compute_edges_weight(const MatrixXd& V, const MatrixXi& F);
void compute_laplacian_matrix(const std::vector<ControlPoint> C);
MatrixXd arap(const MatrixXd& V, const MatrixXi& F, const std::vector<ControlPoint> C, MatrixXd new_V);
