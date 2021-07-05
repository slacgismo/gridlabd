//	File: solvers.h
//	Copyright (C) 2008 Battelle Memorial Institute
//	Copyright (C) 2020 Regents of the Leland Stanford Junior University
//
//	Thermal solvers
//
//	Update with advanced solver in June 2008 by D. Chassin
//	Update with multizone solver in January 202 by D. Chassin

#ifndef _SOLVERS_H
#define _SOLVERS_H

//	Function: e2solve
//
//	Solve the simple ETP 2-zone thermal model
//
//	Returns: time to next event
//
double e2solve( double a,double n,double b,double m,double c,double p=1e-8,double *e=NULL);

//	Structure: s_msolver
//
//	Contains data used by msolve()
//
//	See also: typedef msolver
struct s_msolver
{
	// Member: N
	//
	// The number of zones in the model. This determines the expected
	// size of the remaining members.
	//
	unsigned int N;

	// Member: U
	//
	// Pointer to the N*(N+1)/2 zone-zone conductances
	//
	// Conductances are given in the order of a U-triangular matrix, i.e.
	//	U_01...U_0N, U_12...U_1N, U_23...U_2N, ..., U_(N-1)N
	// where U_0n refers to the conductance between the outdoor air and
	// zone n
	//
	double *U;

	// Member: C
	//
	// Represents the N zone heat heat capacities
	//
	double *C; 

	// Member: q
	// 
	// Disturbance inputs (size N)
	//
	double *q; 

	// Member: T
	//
	// Temperature state (size N+1) in the order T0, T1...TN
	// where T0 is the outdoor air temperature
	//
	double *T; 

	// Member: Tset
	//
	// Temperature setpoint for N-1 controlled zones
	//
	double *Tset;

	// Member: u
	//
	// Control inputs (size N-1) in the order T1..T(N-1).
	// Zone N is the uncontrolled zone.
	//
	double *u; 

	// Member: a
	//
	// Uncontrolled zone heat leakage factor (size 1)
	//
	double *a; 
	
	// Member: umin
	//
	// Control input lower limit (size N-1) in the order T1..T(N-1).
	// Zone N is the uncontrolled zone.
	//
	double *umin; 
	
	// Member: umax
	//
	// Control input upper limit (size N-1) in the order T1..T(N-1).
	// Zone N is the uncontrolled zone.
	//
	double *umax; 

	// Member: Tbal
	//
	// Provides the solution to the autonomous system (equilibrium without control inputs)
	//
	double *Tbal;

	// Member: Teq
	//
	// Provides the solution to the equilibrium system (equilibrium with control inputs)
	double *Teq;

	// Member: dT
	//
	// Provide the dT portion of the solution at t=0
	//
	double *dT;

	// Member: m
	//
	// Provide the implied heating/cooling mode of the solution at t=0
	//
	double *mode;

	// Member: solver
	//
	// This is an internal pointer the solver implementation class
	//
	void *solver;
};

//	Type: msolver
//
//	Contains data used by msolve()
//
//	See also: struct s_msolver
//
typedef struct s_msolver msolver;

//	Function: msolve
//
//	Multizone solver API
//
//	The calling convention is as follows:
//
//	msolve("new")
//		Allocates memory for a new solver
//
//	msolver("set",msolver*,param,index,value)
//		Sets a model parameter to the value given
//		Note that if the param is scalar, index is omitted.
//
//	msolver("update",msolver*)
//		Update the solver internal model after a "set" operation
//		This generates the autonomous and closed loop equilibrium points Tbal and Teq
//
//	msolver("solver",msolver*,dt)
//		Solves the model for the elapsed time dt
//
msolver *msolve(const char *op, ...);

#endif
