#pragma once

#define COMBINE(A, B) A##B

#ifdef _VERBOSE
#define _DEBUG
#define VERBOSE_OUT(VARIABLE) std::cout << "> " << #VARIABLE << ": " << (VARIABLE) << "\n"
#define VERBOSE_OUT_GPU(VARIABLE, COND)                                                            \
    if (COND) printf("> " #VARIABLE ": %f\n", VARIABLE)
#else
#define VERBOSE_OUT(VARIABLE) //
#define VERBOSE_OUT_GPU(VARIABLE, COND)
#endif

#ifdef _DEBUG
#define DEBUG(EXP) EXP
#define DEBUG_OUT(VARIABLE) std::cout << "> " << #VARIABLE << ": " << (VARIABLE) << "\n"
#define DEBUG_OUT_GPU(VARIABLE, COND)                                                              \
    if (COND) printf("> " #VARIABLE ": %f\n", VARIABLE)
#else
#define DEBUG(EXP)
#define DEBUG_OUT(VARIABLE) //
#define DEBUG_OUT_GPU(VARIABLE, COND)
#endif