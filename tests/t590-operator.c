/// @file
/// Test creation, action, and destruction for mass matrix operator at points
/// \test Test creation, action, and destruction for mass matrix operator at points
#include "t590-operator.h"

#include <ceed.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  Ceed    ceed;
  CeedInt num_elem_1d = 3, num_elem = num_elem_1d * num_elem_1d, dim = 2, p = 3, q = 5;
  CeedInt num_nodes = (num_elem_1d * (p - 1) + 1) * (num_elem_1d * (p - 1) + 1), num_points_per_elem = 4, num_points = num_elem * num_points_per_elem;
  CeedVector          x_points, u, v;
  CeedElemRestriction elem_restriction_x_points, elem_restriction_u;
  CeedBasis           basis_u;
  CeedQFunction       qf_mass;
  CeedOperator        op_mass;
  bool                is_at_points;

  CeedInit(argv[1], &ceed);

  CeedVectorCreate(ceed, dim * num_points, &x_points);
  {
    CeedScalar x_array[dim * num_points];

    for (CeedInt e = 0; e < num_elem; e++) {
      for (CeedInt d = 0; d < dim; d++) {
        x_array[num_points_per_elem * (e * dim + d) + 0] = 0.25;
        x_array[num_points_per_elem * (e * dim + d) + 1] = d == 0 ? -0.25 : 0.25;
        x_array[num_points_per_elem * (e * dim + d) + 2] = d == 0 ? 0.25 : -0.25;
        x_array[num_points_per_elem * (e * dim + d) + 3] = 0.25;
      }
    }
    CeedVectorSetArray(x_points, CEED_MEM_HOST, CEED_COPY_VALUES, x_array);
  }
  {
    CeedInt ind_x[num_elem + 1 + num_points];

    for (CeedInt i = 0; i <= num_elem; i++) ind_x[i] = num_elem + 1 + i * num_points_per_elem;
    for (CeedInt i = 0; i < num_points; i++) ind_x[num_elem + 1 + i] = i;
    CeedElemRestrictionCreateAtPoints(ceed, num_elem, num_points, dim, num_points * dim, CEED_MEM_HOST, CEED_COPY_VALUES, ind_x,
                                      &elem_restriction_x_points);
  }

  {
    CeedInt ind_u[num_elem * p * p];

    for (CeedInt e = 0; e < num_elem; e++) {
      CeedInt elem_xy[2] = {1, 1}, n_d[2] = {0, 0};

      for (CeedInt d = 0; d < dim; d++) n_d[d] = num_elem_1d * (p - 1) + 1;
      {
        CeedInt r_e = e;

        for (CeedInt d = 0; d < dim; d++) {
          elem_xy[d] = r_e % num_elem_1d;
          r_e /= num_elem_1d;
        }
      }
      CeedInt num_nodes_in_elem = p * p, *elem_nodes = ind_u + e * num_nodes_in_elem;

      for (CeedInt n = 0; n < num_nodes_in_elem; n++) {
        CeedInt g_node = 0, g_node_stride = 1, r_node = n;

        for (CeedInt d = 0; d < dim; d++) {
          g_node += (elem_xy[d] * (p - 1) + r_node % p) * g_node_stride;
          g_node_stride *= n_d[d];
          r_node /= p;
        }
        elem_nodes[n] = g_node;
      }
    }
    CeedElemRestrictionCreate(ceed, num_elem, p * p, 1, 1, num_nodes, CEED_MEM_HOST, CEED_COPY_VALUES, ind_u, &elem_restriction_u);
  }
  CeedBasisCreateTensorH1Lagrange(ceed, dim, 1, p, q, CEED_GAUSS, &basis_u);

  CeedQFunctionCreateInterior(ceed, 1, mass, mass_loc, &qf_mass);
  CeedQFunctionAddInput(qf_mass, "u", 1, CEED_EVAL_INTERP);
  CeedQFunctionAddOutput(qf_mass, "v", 1, CEED_EVAL_INTERP);

  CeedOperatorCreateAtPoints(ceed, qf_mass, CEED_QFUNCTION_NONE, CEED_QFUNCTION_NONE, &op_mass);
  CeedOperatorSetField(op_mass, "u", elem_restriction_u, basis_u, CEED_VECTOR_ACTIVE);
  CeedOperatorSetField(op_mass, "v", elem_restriction_u, basis_u, CEED_VECTOR_ACTIVE);
  CeedOperatorAtPointsSetPoints(op_mass, elem_restriction_x_points, x_points);

  CeedOperatorIsAtPoints(op_mass, &is_at_points);
  if (!is_at_points) printf("Error: Operator should be at points\n");

  CeedVectorCreate(ceed, num_nodes, &u);
  CeedVectorSetValue(u, 1.0);
  CeedVectorCreate(ceed, num_nodes, &v);
  CeedOperatorApply(op_mass, u, v, CEED_REQUEST_IMMEDIATE);

  {
    CeedScalar        sum = 0.0;
    const CeedScalar *v_array;

    CeedVectorGetArrayRead(v, CEED_MEM_HOST, &v_array);
    for (CeedInt i = 0; i < num_nodes; i++) sum += v_array[i];
    CeedVectorRestoreArrayRead(v, &v_array);
    // Summing 9 reference elements, each 2x2 => 36 sq units area
    if (fabs(sum - 4.0 * num_elem) > CEED_EPSILON * 5e3) printf("Incorrect area computed, %f != %f\n", sum, 4.0 * num_elem);
  }

  CeedVectorDestroy(&x_points);
  CeedVectorDestroy(&u);
  CeedVectorDestroy(&v);
  CeedElemRestrictionDestroy(&elem_restriction_x_points);
  CeedElemRestrictionDestroy(&elem_restriction_u);
  CeedBasisDestroy(&basis_u);
  CeedQFunctionDestroy(&qf_mass);
  CeedOperatorDestroy(&op_mass);
  CeedDestroy(&ceed);
  return 0;
}
