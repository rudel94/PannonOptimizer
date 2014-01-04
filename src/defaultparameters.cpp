/**
 * @file defaultparameters.cpp
 */

#include <defaultparameters.h>

//Linalg Parameters
const double DefaultParameters::E_ABSOLUTE = 1.0E-12;
const double DefaultParameters::E_RELATIVE = 1.0E-10;
const double DefaultParameters::SPARSITY_RATIO = 0.05;
const int DefaultParameters::ELBOWROOM = 5;

//Simplex Parameters
//Tolerances
const double DefaultParameters::E_PIVOT = 1.0E-06;
const double DefaultParameters::E_FEASIBILITY = 1.0E-08;
const double DefaultParameters::E_OPTIMALITY = 1.0E-08;
//Starting procedures
const bool DefaultParameters::PRESOLVE = false;
const bool DefaultParameters::SCALING = false;
const int DefaultParameters::STARTING_BASIS = 0;
//Basis factorization
const int DefaultParameters::FACTORIZATION_TYPE = 0;
const int DefaultParameters::NONTRIANGULAR_METHOD = 2;
const int DefaultParameters::NONTRIANGULAR_PIVOT_RULE = 1;
const int DefaultParameters::REINVERSION_FREQUENCY = 30;
const double DefaultParameters::PIVOT_THRESHOLD = 0.01;
//Pricing
const int DefaultParameters::PRICING_TYPE = 0;
//Ratiotest
const int DefaultParameters::NONLINEAR_PRIMAL_PHASEI_FUNCTION = 1;
const int DefaultParameters::NONLINEAR_DUAL_PHASEI_FUNCTION = 2;
const int DefaultParameters::NONLINEAR_DUAL_PHASEII_FUNCTION = 2;
const int DefaultParameters::THRESHOLD_REPORT_LEVEL = 1;
const int DefaultParameters::EXPAND_DUAL_PHASEI = 0;
const int DefaultParameters::EXPAND_DUAL_PHASEII = 0;
//Global
const int DefaultParameters::DEBUG_LEVEL = 1;
const int DefaultParameters::ITERATION_LIMIT = 200000;
const double DefaultParameters::TIME_LIMIT = 3600.0;

