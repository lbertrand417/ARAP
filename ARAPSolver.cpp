#include "ARAPSolver.h"


// /!\ Ne prend pas en compte les boundaries !!!

std::vector<std::list<int>> neighbors;
MatrixXd weights;
MatrixXd L;

float eps = 1e-10;


/* Find one-ring neighbors of all the vertices in V.
 * V : Matrix of vertices
 * F : Matrix of faces
 *
 * Out : Vector of list of neigbors indices
 */
void find_neighbors(const MatrixXd V, const MatrixXi F) {
    std::vector<std::list<int>> myNeighbors(V.rows());

    for (int i = 0; i < F.rows(); i++) {
        for (int j = 0; j < F.cols(); j++) {
            myNeighbors[F(i, j)].push_back(F(i, (j + 1) % F.cols()));
            myNeighbors[F(i, j)].push_back(F(i, (j + 2) % F.cols()));
        }
    }

    for (int i = 0; i < V.rows(); i++) {
        myNeighbors[i].sort();
        myNeighbors[i].unique();
    }

    neighbors = myNeighbors;

    // DEBUG
    // Print out the vector 
    /*std::cout << "neighbors = { ";
    for (std::list<int> neighbor : neighbors) {
        std::cout << "{ ";
        for (int n : neighbor) {
            std::cout << n << ", ";
        }
        std::cout << "}; \n";
    }
    std::cout << "}; \n";*/
}

/* Compute weights wij
 * V : Matrix of vertices
 * F : Matrix of faces
 *
 * Out : Weights wij = 1/2 * (cot(aij) + cot(bij))
 */
void compute_edges_weight(const MatrixXd& V, const MatrixXi& F) {
    weights = MatrixXd::Zero(V.rows(), V.rows());


    for (int i = 0; i < F.rows(); i++) {
        // Compute edges vectors CCW
        Vector3d v1 = V.row(F(i, 1)) - V.row(F(i, 0));
        Vector3d v2 = V.row(F(i, 2)) - V.row(F(i, 1));
        Vector3d v3 = V.row(F(i, 0)) - V.row(F(i, 2));

        // Compute the angles
        double a201 = acos(v1.dot(-v3) / (v1.norm() * v3.norm()));
        double a012 = acos(-v1.dot(v2) / (v1.norm() * v2.norm()));
        double a120 = acos(-v2.dot(v3) / (v2.norm() * v3.norm())); 

        
        // DEBUG
        /*std::cout << "a120" << std::endl;
        std::cout << a120 * 180 / 3.14 << std::endl;
        std::cout << "a201" << std::endl;
        std::cout << a201 * 180 / 3.14 << std::endl;
        std::cout << "a012" << std::endl;
        std::cout << a012 * 180 / 3.14 << std::endl;*/

        // Add the angles
        weights(F(i, 0), F(i, 1)) += cos(a120) / sin(a120);
        weights(F(i, 1), F(i, 0)) += cos(a120) / sin(a120);

        weights(F(i, 1), F(i, 2)) += cos(a201) / sin(a201);
        weights(F(i, 2), F(i, 1)) += cos(a201) / sin(a201);

        weights(F(i, 2), F(i, 0)) += cos(a012) / sin(a012);
        weights(F(i, 0), F(i, 2)) += cos(a012) / sin(a012);
    }

    // Divide all the weights by 2
    weights = (float) 1 / 2 * weights;

    // Put small values to 0
    for (int i = 0; i < weights.rows(); i++) {
        for (int j = 0; j < weights.cols(); j++) {
            if (weights(i, j) < eps) {
                weights(i, j) = 0;
            }
        }
    }

    // DEBUG
    //std::cout << weights << std::endl;
}

void compute_laplacian_matrix(const std::vector<ControlPoint> C) {
    L = weights;

    // Add constraints
    for (ControlPoint c : C) {
        int index = c.vertexIndexInMesh;
        L.row(index) = VectorXd::Zero(L.cols());
        L.col(index) = VectorXd::Zero(L.rows());
        L(index, index) = 1;
    }

    // Add diagonal value
    for (int i = 0; i < L.rows(); i++) {
        // If it's not a constraint point
        if (L(i, i) != 1) {
            L(i, i) = -L.row(i).sum();
        }
    }

    // DEBUG
    //std::cout << L << std::endl;
}

MatrixXd compute_covariance_matrix(MatrixXd V, MatrixXd new_V, int index) {
    MatrixXd Si(V.cols(), V.cols());

    // Retrieve neighbors of v
    std::list<int> neighbors_v = neighbors[index];   

    // DEBUG
    /*std::cout << "neighbors = { ";
    for (int n : neighbors_v) {
            std::cout << n << ", ";
    }
    std::cout << "}; \n";*/

    MatrixXd P_init = MatrixXd::Zero(V.cols(), neighbors_v.size());
    MatrixXd P_new = MatrixXd::Zero(V.cols(), neighbors_v.size());
    DiagonalMatrix<double, Eigen::Dynamic> D(neighbors_v.size());

    // Vertex of initial and final mesh
    Vector3d v_init = V.row(index);
    Vector3d v_new = new_V.row(index);

    // For each neighbor compute edges (in initial and result meshes)
    int k = 0;
    for (std::list<int>::iterator it = neighbors_v.begin(); it != neighbors_v.end(); ++it, ++k) {
        // Initial mesh
        Vector3d neighbor_init = V.row(*it);
        Vector3d edge_init = v_init - neighbor_init;
        P_init.col(k) = edge_init;

        /*std::cout << "edge_init" << std::endl;
        std::cout << edge_init << std::endl;*/

        // Updated mesh
        Vector3d neighbor_new = new_V.row(*it);
        Vector3d edge_new = v_new - neighbor_new;
        P_new.col(k) = edge_new;

        /*std::cout << "edge_new" << std::endl;
        std::cout << edge_new << std::endl;*/

        // Diagonal mesh
        D.diagonal()[k] = weights(index, *it);
    }



    Si = P_init * D * P_new.transpose();

    // DEBUG
    /*std::cout << "D" << std::endl;
    std::cout << D.diagonal() << std::endl;

    std::cout << "P_init" << std::endl;
    std::cout << P_init << std::endl;

    std::cout << "P_new" << std::endl;
    std::cout << P_new << std::endl;

    std::cout << "Si" << std::endl;
    std::cout << Si << std::endl;*/

    return Si;
}

std::pair<bool, Vector3d> isConstraint(const std::vector<ControlPoint> C, int index) {

    for (ControlPoint c : C) {
        if (index == c.vertexIndexInMesh) {
            return std::pair<bool, Vector3d>(true, c.wantedVertexPosition);
        }
    }
    return std::pair<bool, Vector3d>(false, Vector3d(0,0,0));
}

MatrixXd compute_b(const MatrixXd& V, const std::vector<MatrixXd>& R, const std::vector<ControlPoint> C) {

    MatrixXd b = MatrixXd::Zero(V.rows(), V.cols());

    for (int i = 0; i < V.rows(); i++) {
        VectorXd vi = V.row(i);

        // Retrieve neighbors of v
        std::list<int> neighbors_v = neighbors[i];

        // Rotation matrix of i-th vertex
        MatrixXd Ri = R[i];

        // For each neighbor add the corresponding term
        // Check if the point is a constraint
        std::pair<bool, Vector3d> constraint = isConstraint(C, i);

        if (constraint.first) {
            b.row(i) = (VectorXd) constraint.second;
        }
        else {
            // For each neighbor
            for (std::list<int>::iterator it = neighbors_v.begin(); it != neighbors_v.end(); ++it) {
                std::pair<bool, Vector3d> constraint_n = isConstraint(C, *it);

                // Check if the neighbor is a constraint
                if (!constraint_n.first) {
                    VectorXd neighbor = V.row(*it);

                    // Weight of the edge
                    double wij = weights(i, *it);

                    // Neighbor Rotation matrix
                    MatrixXd Rj = R[*it];

                    b.row(i) -= (double)wij / 2 * (Ri + Rj) * (vi - neighbor);
                }
            }
        }
    }

    return b;
}

/* Apply arap algo for one iteration
 * V : Matrix of initial points (previous frame)
 * C : Constraints vertices 
 *
 * Out : Update V 
 */
MatrixXd arap(const MatrixXd &V, const MatrixXi &F, const std::vector<ControlPoint> C) {
    // Center mesh and constraint points
    MatrixXd V_centered = V.rowwise() - V.colwise().mean();

    std::vector<ControlPoint> C_centered;
    for (ControlPoint c : C) {
        C_centered.push_back(ControlPoint(c.vertexIndexInMesh, c.wantedVertexPosition - V.colwise().mean()));
    }

    // Initialize updated matrix
    MatrixXd new_V = V_centered;

    // DEBUG
    /*std::cout << "C = { ";
    for (ControlPoint c : C) {
        std::cout << c.vertexIndexInMesh << ": ";
        std::cout << c.wantedVertexPosition << ", ";
    }
    std::cout << "}; \n";*/


    // Compute weights
    compute_edges_weight(V, F);

    // Precompute Laplacian-Beltrami matrix
    compute_laplacian_matrix(C);

    // ITERATE
    for (int k = 0; k < 10; k++) {

        // Find optimal Ri for each cell
        std::vector<MatrixXd> R(V.rows()); // Matrix of local rotations
        for (int i = 0; i < V.rows(); i++) {
            MatrixXd Si = compute_covariance_matrix(V_centered, new_V, i);

            JacobiSVD<MatrixXd> svd(Si, ComputeThinU | ComputeThinV);

            DiagonalMatrix<double, 3> D(1, 1, (svd.matrixV() * svd.matrixU().transpose()).determinant());
            MatrixXd Ri = svd.matrixV() * D * svd.matrixU().transpose();

            // Store Ri
            R[i] = Ri;

            // DEBUG
            /*std::cout << "Ri" << std::endl;
            std::cout << Ri << std::endl;*/
        }


        // Find optimal p'
        MatrixXd b = compute_b(V_centered, R, C_centered);

        new_V = L.ldlt().solve(b);

        // Shift the new centroid to the previous centroid of the mesh (is it correct?)
        VectorXd mean = VectorXd::Zero(3);
        for (int i = 0; i < new_V.rows(); i++) {
            std::pair<bool, Vector3d> constraint = isConstraint(C, i);
            if (!constraint.first) {
                mean += new_V.row(i);
            }
        }
        mean /= (new_V.rows() - C.size());

        // R[i] nécessaire dans la translation ??
        for (int i = 0; i < new_V.rows(); i++) {
            std::pair<bool, Vector3d> constraint = isConstraint(C, i);
            if (!constraint.first) {
                new_V.row(i) += -mean + V.colwise().mean().transpose();
            }
            else {
                new_V.row(i) += V.colwise().mean().transpose();
            }
        }
    }
    



    return new_V;
}