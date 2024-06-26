// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and other CEED contributors.
// All Rights Reserved. See the top-level LICENSE and NOTICE files for details.
//
// SPDX-License-Identifier: BSD-2-Clause
//
// This file is part of CEED:  http://github.com/ceed

#include <ceed.h>
#include <ceed/backend.h>
#include <hip/hip_runtime.h>

#include "../hip/ceed-hip-common.h"
#include "ceed-hip-gen.h"

//------------------------------------------------------------------------------
// Apply QFunction
//------------------------------------------------------------------------------
static int CeedQFunctionApply_Hip_gen(CeedQFunction qf, CeedInt Q, CeedVector *U, CeedVector *V) {
  return CeedError(CeedQFunctionReturnCeed(qf), CEED_ERROR_BACKEND, "Backend does not implement QFunctionApply");
}

//------------------------------------------------------------------------------
// Destroy QFunction
//------------------------------------------------------------------------------
static int CeedQFunctionDestroy_Hip_gen(CeedQFunction qf) {
  CeedQFunction_Hip_gen *data;

  CeedCallBackend(CeedQFunctionGetData(qf, &data));
  CeedCallHip(CeedQFunctionReturnCeed(qf), hipFree(data->d_c));
  CeedCallBackend(CeedFree(&data->qfunction_source));
  CeedCallBackend(CeedFree(&data));
  return CEED_ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
// Create QFunction
//------------------------------------------------------------------------------
int CeedQFunctionCreate_Hip_gen(CeedQFunction qf) {
  Ceed                   ceed;
  CeedQFunction_Hip_gen *data;

  CeedCallBackend(CeedQFunctionGetCeed(qf, &ceed));
  CeedCallBackend(CeedCalloc(1, &data));
  CeedCallBackend(CeedQFunctionSetData(qf, data));

  // Read QFunction source
  CeedCallBackend(CeedQFunctionGetKernelName(qf, &data->qfunction_name));
  CeedDebug256(ceed, CEED_DEBUG_COLOR_SUCCESS, "----- Loading QFunction User Source -----\n");
  CeedCallBackend(CeedQFunctionLoadSourceToBuffer(qf, &data->qfunction_source));
  CeedDebug256(ceed, CEED_DEBUG_COLOR_SUCCESS, "----- Loading QFunction User Source Complete! -----\n");
  CeedCheck(data->qfunction_source, ceed, CEED_ERROR_UNSUPPORTED, "/gpu/hip/gen backend requires QFunction source code file");

  CeedCallBackend(CeedSetBackendFunction(ceed, "QFunction", qf, "Apply", CeedQFunctionApply_Hip_gen));
  CeedCallBackend(CeedSetBackendFunction(ceed, "QFunction", qf, "Destroy", CeedQFunctionDestroy_Hip_gen));
  return CEED_ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
