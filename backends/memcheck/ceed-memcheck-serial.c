// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and other CEED contributors.
// All Rights Reserved. See the top-level LICENSE and NOTICE files for details.
//
// SPDX-License-Identifier: BSD-2-Clause
//
// This file is part of CEED:  http://github.com/ceed

#include <ceed.h>
#include <ceed/backend.h>
#include <string.h>

#include "ceed-memcheck.h"

//------------------------------------------------------------------------------
// Backend Init
//------------------------------------------------------------------------------
static int CeedInit_Memcheck(const char *resource, Ceed ceed) {
  if (strcmp(resource, "/cpu/self/memcheck") && strcmp(resource, "/cpu/self/memcheck/serial")) {
    // LCOV_EXCL_START
    return CeedError(ceed, CEED_ERROR_BACKEND, "Valgrind Memcheck backend cannot use resource: %s", resource);
    // LCOV_EXCL_STOP
  }

  // Create reference Ceed that implementation will be dispatched through unless overridden
  Ceed ceed_ref;
  CeedCallBackend(CeedInit("/cpu/self/ref/serial", &ceed_ref));
  CeedCallBackend(CeedSetDelegate(ceed, ceed_ref));

  CeedCallBackend(CeedSetBackendFunction(ceed, "Ceed", ceed, "VectorCreate", CeedVectorCreate_Memcheck));
  CeedCallBackend(CeedSetBackendFunction(ceed, "Ceed", ceed, "QFunctionCreate", CeedQFunctionCreate_Memcheck));
  CeedCallBackend(CeedSetBackendFunction(ceed, "Ceed", ceed, "QFunctionContextCreate", CeedQFunctionContextCreate_Memcheck));

  return CEED_ERROR_SUCCESS;
}

//------------------------------------------------------------------------------
// Backend Register
//------------------------------------------------------------------------------
CEED_INTERN int CeedRegister_Memcheck_Serial(void) { return CeedRegister("/cpu/self/memcheck/serial", CeedInit_Memcheck, 100); }
//------------------------------------------------------------------------------
