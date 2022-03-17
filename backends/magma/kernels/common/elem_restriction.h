// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and other CEED contributors.
// All Rights Reserved. See the top-level LICENSE and NOTICE files for details.
//
// SPDX-License-Identifier: BSD-2-Clause
//
// This file is part of CEED:  http://github.com/ceed

#ifndef CEED_MAGMA_ELEM_RESTRICTION_DEVICE_H
#define CEED_MAGMA_ELEM_RESTRICTION_DEVICE_H

#ifdef HAVE_HIP
#include "../hip/atomics.hip.h"
#else
#include "../cuda/atomics.cuh"
#endif

#define MAGMA_ERSTR_THREADS 256
//////////////////////////////////////////////////////////////////////////////////////////
// Fastest index listed first
// i : related to nodes
// e : elements
// c: component
// Go from L-vector (du) to E-vector (dv):
//
// dv(i, e, c) = du( offsets(i, e) + compstride * c)  
static __launch_bounds__(MAGMA_ERSTR_THREADS) __global__ void 
magma_readDofsOffset_kernel(const int NCOMP, const int compstride,
                            const int esize, const int nelem, int *offsets, 
                            const CeedScalar *du, CeedScalar *dv)
{
  const int  pid = threadIdx.x;
  const int elem = blockIdx.x;
 
  for (CeedInt i = pid; i < esize; i += blockDim.x) {
        const CeedInt ind = offsets ? offsets[i + elem * esize] : i + elem * esize;
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            dv[i+elem*esize+comp*esize*nelem] = du[ind + compstride * comp];
        }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Fastest index listed first
// i : related to nodes
// e : elements
// c: component
// Go from L-vector (du) to E-vector (dv), with strides provided 
//  to describe the L-vector layout
//
// dv(i, e, c) = du( i * strides[0] + c * strides[1] + e * strides[2] )  
static __launch_bounds__(MAGMA_ERSTR_THREADS) __global__ void 
magma_readDofsStrided_kernel(const int NCOMP, const int esize, const int nelem,
                             const int *strides, const CeedScalar *du, CeedScalar *dv)
{
  const int  pid = threadIdx.x;
  const int elem = blockIdx.x;
 
  for (CeedInt i = pid; i < esize; i += blockDim.x) {
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            dv[i+elem*esize+comp*esize*nelem] = du[i * strides[0] + 
                                                   comp * strides[1] + 
                                                   elem * strides[2]];
        }
  }
}

// Fastest index listed first
// i : related to nodes
// e : elements
// c: component
// Go from E-vector (du) to L-vector (dv):
//
// dv(offsets(i, e) + compstride * c) = du(i, e, c)
// Double precision version (calls magma_datomic_add)
static __launch_bounds__(MAGMA_ERSTR_THREADS) __global__ void 
magma_writeDofsOffset_kernel_d(const int NCOMP, const int compstride,
                               const int esize, const int nelem, int *offsets, 
                               const double *du, double *dv)
{
    const int  pid = threadIdx.x;
    const int elem = blockIdx.x;

    for (CeedInt i = pid; i < esize; i += blockDim.x) {
        const CeedInt ind = offsets ? offsets[i + elem * esize] : i + elem * esize;
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            magmablas_datomic_add(dv + (ind + compstride * comp),
                                  du[i+elem*esize+comp*esize*nelem]);
        }
    }
}
// Single precision version (calls magma_satomic_add)
static __global__ void 
magma_writeDofsOffset_kernel_s(const int NCOMP, const int compstride,
                               const int esize, const int nelem, int *offsets, 
                               const float *du, float *dv)
{
    const int  pid = threadIdx.x;
    const int elem = blockIdx.x;

    for (CeedInt i = pid; i < esize; i += blockDim.x) {
        const CeedInt ind = offsets ? offsets[i + elem * esize] : i + elem * esize;
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            magmablas_satomic_add(dv + (ind + compstride * comp),
                                  du[i+elem*esize+comp*esize*nelem]);
        }
    }
}

// Fastest index listed first
// i : related to nodes
// e : elements
// c: component
// Go from E-vector (du) to L-vector (dv), with strides provided 
//  to describe the L-vector layout
//
// dv( i * strides[0] + c * strides[1] + e * strides[2] ) = du(i, e, c) 
// Double precision version (calls magma_datomic_add)
static __launch_bounds__(MAGMA_ERSTR_THREADS) __global__ void 
magma_writeDofsStrided_kernel_d(const int NCOMP, const int esize, const int nelem,
                                const int *strides, const double *du, double *dv)
{
    const int  pid = threadIdx.x;
    const int elem = blockIdx.x;

    for (CeedInt i = pid; i < esize; i += blockDim.x) {
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            magmablas_datomic_add(dv + (i * strides[0] + comp * strides[1] + 
                                        elem * strides[2]),
                                  du[i+elem*esize+comp*esize*nelem]);
        }
    }
}

// Single precision version (calls magma_satomic_add)
static __global__ void 
magma_writeDofsStrided_kernel_s(const int NCOMP, const int esize, const int nelem,
                                const int *strides, const float *du, float *dv)
{
    const int  pid = threadIdx.x;
    const int elem = blockIdx.x;

    for (CeedInt i = pid; i < esize; i += blockDim.x) {
        for (CeedInt comp = 0; comp < NCOMP; ++comp) {
            magmablas_satomic_add(dv + (i * strides[0] + comp * strides[1] + 
                                        elem * strides[2]),
                                  du[i+elem*esize+comp*esize*nelem]);
        }
    }
}

#endif    // CEED_MAGMA_ELEM_RESTRICTION_DEVICE_H
