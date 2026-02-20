///////////////////////////
// The sample PSO plugin uses
// "Standard PSO 2007" implementation by Maurice Clerc
// as a starting point
// Used with permission of the original author.
// 
// Original sources available from:
// http://www.particleswarm.info/Programs.html
// 
// Modifications were needed because
// Original Standard PSO is intended to be run from command line only
// and runs out of stack space under normal settings
// We also needed to pass the parameters from AmiBroker structures
// back and forth to SPSO code and we needed to call
// AmiBroker Evaluate callback function from inside SPSO's perf()
// Also poor, built-in C random number generator was replaced by kiss_rand()
//
// The original can be found in SPSO2007.bak file in the project folder
//
// -- Tomasz Janeczko, amibroker.com

#include <stdafx.h>
#include "plugin.h"

#include "SPSO2007.h" // headers for global stuff used in SPSO2007.cpp

#define ulong unsigned long // To generate pseudo-random numbers
#define RAND_MAX_KISS ((unsigned long) 4294967295) 
ulong	rand_kiss();
void	seed_rand_kiss(ulong seed); 



/*
Standard PSO 2007
 Contact for remarks, suggestions etc.:
 MauriceClerc@WriteMe.com
 
 Last update
 2007-12-10 Warning about rotational invariance (valid here only on 2D)
 2007-11-22 stop criterion (option): distance to solution < epsilon
 			and log_progress evaluation
 2007-11-21 Ackley function
 
  -------------------------------- Contributors 
 The works and comments of the following persons have been taken
 into account while designing this standard.  Sometimes this is for 
 including a feature, and sometimes for leaving out one. 
 
 Auger, Anne
 Blackwell, Tim
 Bratton, Dan
 Clerc, Maurice
 Croussette, Sylvain 
 Dattasharma, Abhi
 Eberhart, Russel
 Hansen, Nikolaus
 Keko, Hrvoje
 Kennedy, James 
 Krohling, Renato
 Langdon, William
 Liu, Hongbo 
 Miranda, Vladimiro
 Poli, Riccardo
 Serra, Pablo
 Stickel, Manfred
 
 -------------------------------- Motivation
Quite often, researchers claim to compare their version of PSO 
with the "standard one", but the "standard one" itself seems to vary!
Thus, it is important to define a real standard that would stay 
unchanged for at least one year.

This PSO version does not intend to be the best one on the market
(in particular, there is no adaptation of the swarm size nor of the
coefficients). This is simply very near to the original version (1995),
with just a few improvements based on some recent works.

 --------------------------------- Metaphors
swarm: A team of communicating people (particles)
At each time step
    Each particle chooses a few informants at random, selects the best
	one from this set, and takes into account the information given by
	the chosen particle.
	If it finds no particle better than itself, then the "reasoning" is:
	"I am the best, so I just take my current velocity and my previous
	best position into account" 

----------------------------------- Parameters/Options
clamping := true/false => whether to use clamping positions or not
randOrder:= true/false => whether to avoid the bias due to the loop
				on particles "for s = 1 to swarm_size ..." or not
rotation := true/false => whether the algorithm is sensitive 
				to a rotation of the landscape or not 

You may also modify the following ones, although suggested values
are either hard coded or automatically computed:
S := swarm size
K := maximum number of particles _informed_ by a given one
w := first cognitive/confidence coefficient
c := second cognitive/confidence coefficient

 ----------------------------------- Equations
For each particle and each dimension
Equation 1:	v(t+1) = w*v(t) + R(c)*(p(t)-x(t)) + R(c)*(g(t)-x(t))
Equation 2:	x(t+1) = x(t) + v(t+1)
where
v(t) := velocity at time t
x(t) := position at time t
p(t) := best previous position of the particle
g(t) := best position amongst the best previous positions
		of the informants of the particle
R(c) := a number coming from a random distribution, which depends on c
In this standard, the distribution is uniform on [0,c]

Note 1:
When the particle has no informant better than itself,
it implies p(t) = g(t)

Therefore, Equation 1 gets modified to:
v(t+1) = w*v(t) + R(c)*(p(t)-x(t))

Note 2:
When the "non sensitivity to rotation" option is activated
(p(t)-x(t)) (and (g(t)-x(t))) are replaced by rotated vectors, 
so that the final DNPP (Distribution of the Next Possible Positions)
is not dependent on the system of co-ordinates.

 ----------------------------------- Information links topology 
A lot of work has been done about this topic. The main result is this: 
There is no "best" topology. Hence the random approach used here.  

 ----------------------------------- Initialisation
Initial positions are chosen at random inside the search space 
(which is supposed to be a hyperparallelepiped, and often even
a hypercube), according to a uniform distribution.
This is not the best way, but the one used in the original PSO.

Each initial velocity is simply defined as the half-difference of two
random positions. It is simple, and needs no additional parameter.
However, again, it is not the best approach. The resulting distribution
is not even uniform, as is the case for any method that uses a
uniform distribution independently for each component.

The mathematically correct approach needs to use a uniform
distribution inside a hypersphere. It is not very difficult,
and was indeed used in some PSO versions.  However, it is quite
different from the original one. 
Moreover, it may be meaningless for some heterogeneous problems,
when each dimension has a different "interpretation".

------------------------------------ From SPSO-06 to SPSO-07
The main differences are:

1. option "non sensitivity to rotation of the landscape"
	Note: although theoretically interesting, this option is quite
        computer time consuming, and the improvement in result may
		only be marginal. 

2. option "random permutation of the particles before each iteration"
	Note: same remark. Time consuming, no clear improvement

3. option "clamping position or not"
	Note: in a few rare cases, not clamping positions may induce an
	infinite run, if the stop criterion is the maximum number of 
	evaluations
		
4. probability p of a particular particle being an informant of another
	particle. In SPSO-06 it was implicit (by building the random infonetwork)
	Here, the default value is directly computed as a function of (S,K),
	so that the infonetwork is exactly the same as in SPSO-06.
	However, now it can be "manipulated" ( i.e. any value can be assigned)
	
5. The search space can be quantised (however this algorithm is _not_
   for combinatorial problems)

Also, the code is far more modular. It means it is slower, but easier
to translate into another language, and easier to modify.


 ----------------------------------- Use
 Define the problem (you may add your own one in problemDef() and perf())
 Choose your options
 Run and enjoy!
   
 ------------------------------------ Some results
 So that you can check your own implementation, here are some results.
 
(clamping, randOrder, rotation, stop) = (1,1,1,0) 
Function	Domain			error	Nb_of_eval	Nb_of_runs	Result
Parabola	[-100,100]^30	0.0001	6000		100			 52% (success rate)
	shifted		""			""		""			""			 7%	
	shifted		""			""		7000		""			 100% 
Griewank	[-100,100]^30	0.0001	9000		100			 51%
Rosenbrock	[-10,10]^30		0.0001	40000		50			 31
Rastrigin	[-10,10]^30		0.0001	40000		50			 53 (error mean)
Tripod		[-100,100]^2	0.0001	10000		50			 50%

(clamping, randOrder, rotation, stop) = (1,1,0,0) 
Parabola	[-100,100]^30	0.0001	6000		100			 0.69 (0%) 
	shifted		""			""		""			""			 2.16 (0%)			
Griewank	[-100,100]^30	0.0001	9000		100			 14%
Rosenbrock	[-10,10]^30		0.0001	40000		100			 38.7
Rastrigin	[-10,10]^30		0.0001	40000		100			 54.8 (error mean)
Tripod		[-100,100]^2	0.0001	10000		100			 47%


Because of randomness, and also depending on your own pseudo-random 
number generator, you just have to find similar values, 
not exactly these ones. 
 */  
  
#include "stdio.h"
#include "math.h"
#include <stdlib.h>
#include <time.h>
  
#define	D_max 100		// Max number of dimensions of the search space
#define R_max 200		// Max number of runs
#define	S_max 100		// Max swarm size
#define zero  0			// 1.0e-30 // To avoid numerical instabilities

  // Structures
struct quantum 
{
  double q[D_max];
  int size; 
};

struct SS 
{ 
	int D;
	double max[D_max];
	double min[D_max]; 
	struct quantum q;		// Quantisation step size. 0 => continuous problem
};

struct param 
{
  	double c;		// Confidence coefficient
  	int clamping;	// Position clamping or not
	int K;			// Max number of particles informed by a given one
	double p;		// Probability threshold for random topology	
					// (is actually computed as p(S,K) )
	int randOrder;	// Random choice of particles or not
	int rotation;	// Sensitive to rotation or not
  	int S;			// Swarm size
  	int stop;		// Flag for stop criterion
  	double w;		// Confidence coefficient
};

struct position 
{ 
	double f;  
	int improved;   
	int size;  
	double x[D_max];
};

struct velocity 
{  
	int size;  
	double v[D_max]; 
};

struct problem 
{ 
	double epsilon; 	// Admissible error
	int evalMax; 		// Maximum number of fitness evaluations
	int function; 		// Function code
	double objective; 	// Objective value
						// Solution position (if known, just for tests)	
	struct position solution;

	struct SS SS;		// Search space
};


struct swarm 
{ 
	int best; 					// rank of the best particle
	struct position P[S_max];	// Previous best positions found by each particle
	int S; 						// Swarm size 
	struct velocity V[S_max];	// Velocities
  	struct position X[S_max];	// Positions 
};

struct result 
{  
	double nEval; 		// Number of evaluations  
	struct swarm SW;	// Final swarm
	double error;		// Numerical result of the run
};

struct matrix 	// Useful for "non rotation sensitive" option
{  
	int size;  
	double v[D_max][D_max]; 
};


  // Sub-programs
double alea (double a, double b);
int alea_integer (int a, int b);
double alea_normal (double mean, double stdev);
double distanceL(struct position x1, struct position x2, double L);
struct velocity aleaVector(int D,double coeff);
struct matrix matrixProduct(struct matrix M1,struct matrix M2);
struct matrix matrixRotation(struct velocity V);
struct velocity	matrixVectProduct(struct matrix M,struct velocity V);
double normL (struct velocity v,double L);
double perf (struct position x, int function,struct SS SS);	// Fitness evaluation
struct position quantis (struct position x, struct SS SS);
struct problem problemDef(int functionCode);
struct result PSO ( struct param param, struct problem problem);
int sign (double x);

  // Global variables
long double E;			// exp(1). Useful for some test functions
long double pi;			// Useful for some test functions
struct matrix reflex1;
long double sqrtD;

  // File(s);
FILE * f_run;
FILE * f_synth;

// =================================================
// TJ: We can't have main() function in a DLL, 
// it is splitted into 3 parts: init(), optimize() and final() functions
//
// main() --- removed


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// TJ:
// We take variable def that were inside main() and 
// move to global level, so we can access from other functions
// too as DLL will be called from the outside and
// needs access to those variables

int d;			// Current dimension
double error;			// Current error
double errorMean;		// Average error
double errorMin;		// Best result over all runs
double errorMeanBest[R_max]; 
double evalMean;		// Mean number of evaluations
int functionCode;
int i,j;
int nFailure;		// Number of unsuccessful runs
double logProgressMean;
struct param param;
struct problem pb; 
int run;
int runMax = 5;
int evalMax = 1000; 
struct result result; 
double successRate;
double variance;

// TJ:
// our init() function is a replacement of first part of main()
// function
void init() 
{ 
	seed_rand_kiss(1); 
	
	//f_run = fopen ("f_run.txt", "w");  
	//f_synth = fopen ("f_synth.txt", "w"); 
		
	E = exp ((long double) 1); 
	pi = acos ((long double) -1);
		
			
// ----------------------------------------------- PROBLEM
	functionCode = 0;	
			/* (see problemDef( ) for precise definitions)
				 0 Parabola (Sphere)
				 1 Griewank
				 2 Rosenbrock (Banana)
				 3 Rastrigin
				 4 Tripod (dimension 2)
				 5 Ackley
				100 Shifted Parabola 
				99 Test
			 */ 
	run = 0;	
	if (runMax > R_max) runMax = R_max;
			
				
	// -----------------------------------------------------
	// PARAMETERS
	// * means "suggested value"		
			
	param.clamping =1;
			// 0 => no clamping AND no evaluation. WARNING: the program
			// 				may NEVER stop (in particular with option move 20 (jumps)) *
			// *1 => classical. Set to bounds, and velocity to zero
		
	param.randOrder=1; // 0 => at each iteration, particles are modified
						//     always according to the same order 0..S-1
						//*1 => at each iteration, particles numbers are
						//		randomly permutated

	param.rotation=0; // WARNING. Quite time consuming!
	// WARNING. Experimental code, completely valid only for dimension 2
	// 0 =>  sensitive to rotation of the system of coordinates
	//*1 => non sensitive (except side effects)
	// by using a rotated hypercube for the probability distribution
			
	param.stop = 0;	// Stop criterion
					// 0 => error < pb.epsilon
					// 1 => eval < pb.evalMax		
					// 2 => ||x-solution|| < pb.epsilon
					
		
	// -------------------------------------------------------
	// Some information
	TRACE ("\n Function %i ", functionCode);
	TRACE ("\n (clamping, randOrder, rotation, stop_criterion) = (%i, %i, %i, %i)",
		param.clamping, param.randOrder, param.rotation, param.stop);
		
	// =========================================================== 
	// RUNs
			
	// Initialize some objects

	pb=problemDef(functionCode);
	
	// You may "manipulate" S, p, w and c
	// but here are the suggested values
	param.S = (int) (10 + 2 * sqrt( (double)pb.SS.D));	// Swarm size
	if (param.S > S_max) param.S = S_max;
		
	printf("\n Swarm size %i", param.S);
	
	param.K=3; 											
	param.p=1-pow(1-(double)1/(param.S),param.K); 
	
	// According to Clerc's Stagnation Analysis
	param.w = 1 / (2 * log ((double) 2)); // 0.721
	param.c = 0.5 + log ((double) 2); // 1.193
		
	// According to Poli's Sampling Distribution of PSOs analysis	
	//param.w = ??; // in [0,1[
	//param.c =  
	//    smaller than 12*(param.w*param.w-1)/(5*param.w -7); 
	
	
	TRACE ("\n c = %f,  w = %f",param.c, param.w);
	
	//---------------
	sqrtD=sqrt((long double) pb.SS.D);
		
	// Define just once the first reflexion matrix
	if(param.rotation>0)
	{					
		reflex1.size=pb.SS.D;	
		for (i=0;i<pb.SS.D;i++)
		{
			for (j=0;j<pb.SS.D;j++)
			{
				reflex1.v[i][j]=-2.0/pb.SS.D;
			}
		}
	
		for (d=0;d<pb.SS.D;d++)
		{
			reflex1.v[d][d]=1+reflex1.v[d][d];
		}
	}
								
	errorMean = 0;	    
	evalMean = 0;	    
	nFailure = 0;	

	// the END of initialization phase
	// now it is the time to iterate PSO()
	// but that will be called from optimize
	
}


// TJ:
// the optimize() function is called
// multiple times from DLL's OptimizerRun() 
//
// it performs actual optimization step
//
// it is functional equivalent of middle part of original main() function
//
//  That 'for' is taken from original, but not needed
//  as iteration is done by AmiBroker 
//	//------------------------------------- RUNS	
//	for (run = 0; run < runMax; run++)  

void optimize()
{	
	result = PSO (param, pb);
	error = result.error;
	
	if (error > pb.epsilon) // Failure
	{
		nFailure = nFailure + 1;
	}

	
		// Result display

	TRACE ("\nRun %i. Eval %f. Error %f ", run++, result.nEval, error);
	TRACE("  x(0)= %f",result.SW.P[result.SW.best].x[0]);
		// Save result
		// fprintf( f_run, "\n%i %.0f %f ", run, result.nEval,  error );
		// for ( d = 0; d < SS.D; d++ ) fprintf( f_run, " %f",  bestResult.x[d] );
		
		// Compute/store some statistical information
	if (run == 0)
		errorMin = error;
	else if (error < errorMin)
		errorMin = error;

	evalMean = evalMean + result.nEval;	
	errorMean = errorMean + error;	
	errorMeanBest[run] = error;
	logProgressMean  = logProgressMean - log(error);		

	// End loop on "run"
}	
			

// TJ:
// the final() function
// replaces the ending part of main()
void final()
{
	// ---------------------END 
	// Display some statistical information
	
	evalMean = evalMean / (double) runMax;   
	errorMean = errorMean / (double) runMax;
	logProgressMean = logProgressMean/(double) runMax;
			
	printf ("\n Eval. (mean)= %f", evalMean);	
	printf ("\n Error (mean) = %f", errorMean);
		
	// Variance
	variance = 0;
			
	for (run = 0; run < runMax; run++)
				variance = variance + pow (errorMeanBest[run] - errorMean, 2);
			
	variance = sqrt (variance / runMax);	    
	TRACE ("\n Std. dev. %f", variance); 
	TRACE("\n Log_progress (mean) = %f", logProgressMean);	

	// Success rate and minimum value
	TRACE("\n Failure(s) %i",nFailure);
	successRate = 100 * (1 - nFailure / (double) runMax);			
	TRACE ("\n Success rate = %.2f%%", successRate);
			
	if (run > 1)
		TRACE ("\n Best min value = %f", errorMin);
			
	// Save
	/*
		fprintf(f_synth,"\n"); for (d=0;d<SS.D;d++) fprintf(f_synth,"%f ",
			pb.offset[d]);
		fprintf(f_synth,"    %f %f %f %.0f%% %f",errorMean,variance,errorMin,
			successRate,evalMean); 

	 fprintf( f_synth, "\n%f %f %f %f %.0f%% %f ", shift,
			errorMean, variance, errorMin, successRate, evalMean );
	*/
	/*
	fprintf (f_synth, "\n");		
	fprintf (f_synth, "%f %f %.0f%% %f   ",
					errorMean, variance, successRate, evalMean);		   
			
	*/
	// TJ:
	// The original code did not close the files
	// it must be corrected:
	//fclose( f_synth );
	//fclose( f_run );

}

// TJ:
// Due to stack overflow that PSO generated, it was required
// to make all structs and array declarations as static
// to use globally allocated memory instead of stack 
// As PSO() is NOT called recurrently, it is OK to do so.
// ===============================================================
// PSO
struct result PSO (struct param param, struct problem pb) 
{  
	static struct velocity aleaV;  
	int d;   
	double error;   
	double errorPrev;
	static struct velocity expt1,expt2;
	int g;  
	static struct velocity GX;   
	static int index[S_max], indexTemp[S_max];     
	int initLinks;	// Flag to (re)init or not the information links
	int iter; 		// Iteration number (time step)
	int iterBegin;
	int length;
	static int LINKS[S_max][S_max];	// Information links
	int m; 
	int noEval; 	
	double normPX, normGX;
	int noStop;
	int outside;
	double p;
	static struct velocity PX;	
	static struct result R;
	int rank;
	static struct matrix RotatePX;
	static struct matrix RotateGX;
	int s0, s,s1; 
	int t;
	double zz;
	
	aleaV.size=pb.SS.D;
	RotatePX.size=pb.SS.D;
	RotateGX.size=pb.SS.D;	
	
	// -----------------------------------------------------
	// INITIALISATION
	p=param.p; // Probability threshold for random topology
	R.SW.S = param.S; // Size of the current swarm
			
	// Position and velocity

	for (s = 0; s < R.SW.S; s++)   
	{
		R.SW.X[s].size = pb.SS.D;
		R.SW.V[s].size = pb.SS.D;
			
		for (d = 0; d < pb.SS.D; d++)  
		{  
			R.SW.X[s].x[d] = alea (pb.SS.min[d], pb.SS.max[d]);
		}
			
		for (d = 0; d < pb.SS.D; d++)  
		{				
			R.SW.V[s].v[d] = 
			(alea( pb.SS.min[d], pb.SS.max[d] ) - R.SW.X[s].x[d])/2; 
		}

		// Take quantisation into account
		R.SW.X[s] = quantis (R.SW.X[s], pb.SS);
	}
		
				
	// First evaluations
	for (s = 0; s < R.SW.S; s++) 
	{	

		R.SW.X[s].f =
			perf (R.SW.X[s], pb.function,pb.SS);
		
		R.SW.P[s] = R.SW.X[s];	// Best position = current one
		R.SW.P[s].improved = 0;	// No improvement
	}
	
	// If the number max of evaluations is smaller than 
	// the swarm size, just keep evalMax particles, and finish
	if (R.SW.S>pb.evalMax) R.SW.S=pb.evalMax;	
	R.nEval = R.SW.S;
	 
	// Find the best
	R.SW.best = 0;
	switch (param.stop)
	{
		default:
		errorPrev = fabs(pb.epsilon-R.SW.P[R.SW.best].f);
		break;
		
		case 2:
		errorPrev=distanceL(R.SW.P[R.SW.best],pb.solution,2);
		break;
	}		
		
	for (s = 1; s < R.SW.S; s++)     
	{
		switch (param.stop)
		{
			default:
			zz=fabs(pb.epsilon-R.SW.P[s].f);
			if (zz < errorPrev)
				R.SW.best = s;
				errorPrev=zz;			
			break;
			
			case 2:
			zz=distanceL(R.SW.P[R.SW.best],pb.solution,2);
			if (zz<errorPrev)
				R.SW.best = s;
				errorPrev=zz;			
			break;		
		}   
	}
	
	
    
/*		
	// Display the best
	printf( " Best value after init. %f ", errorPrev );
	printf( "\n Position :\n" );
	for ( d = 0; d < SS.D; d++ ) printf( " %f", R.SW.P[R.SW.best].x[d] );
*/	 
	initLinks = 1;		// So that information links will beinitialized
			// Note: It is also a flag saying "No improvement"
	noStop = 0;	
			
	// ---------------------------------------------- ITERATIONS
	iter=0; iterBegin=0;
	while (noStop == 0) 
	{
		iter=iter+1;
		
		if (initLinks==1)	// Random topology
		{
			// Who informs who, at random

			for (s = 0; s < R.SW.S; s++)
			{	
				for (m = 0; m < R.SW.S; m++)
				{		    
					if (alea (0, 1)<p) LINKS[m][s] = 1;	// Probabilistic method
					else LINKS[m][s] = 0;
				}
			}

			// Each particle informs itself
			for (m = 0; m < R.SW.S; m++)
			{
					LINKS[m][m] = 1;	     
			}	  
		}
						
		// The swarm MOVES
		//printf("\nIteration %i",iter);
		for (s = 0; s < R.SW.S; s++)  index[s]=s;
					
		switch (param.randOrder)
		{
			default:
			break;
		
			case 1: //Define a random permutation
			length=R.SW.S;
			for (s=0;s<length;s++) indexTemp[s]=index[s];
					
			for (s=0;s<R.SW.S;s++)
			{
				rank=alea_integer(0,length-1);
				index[s]=indexTemp[rank];
				if (rank<length-1)	// Compact
				{
					for (t=rank;t<length;t++)
						indexTemp[t]=indexTemp[t+1];
				}					
						length=length-1;
			}
			break;			
		}

		for (s0 = 0; s0 < R.SW.S; s0++)	// For each particle ...
		{	
			s=index[s0];
			// ... find the first informant
			s1 = 0;    
			while (LINKS[s1][s] == 0)	s1++;					
			if (s1 >= R.SW.S)	s1 = s;
				
		// Find the best informant			
		g = s1;	
		for (m = s1; m < R.SW.S; m++) 
		{	    
			if (LINKS[m][s] == 1 && R.SW.P[m].f < R.SW.P[g].f)
					g = m;
		}	
						
		//.. compute the new velocity, and move
		
		// Exploration tendency
		for (d = 0; d < pb.SS.D; d++)
		{
			R.SW.V[s].v[d]=param.w *R.SW.V[s].v[d];
		}
												
		// Prepare Exploitation tendency
		for (d = 0; d < pb.SS.D; d++)
		{
			PX.v[d]= R.SW.P[s].x[d] - R.SW.X[s].x[d];
		}				
		PX.size=pb.SS.D; 
		
		if(g!=s)
		{
			for (d = 0; d < pb.SS.D; d++)
			{
				GX.v[d]= R.SW.P[g].x[d] - R.SW.X[s].x[d];
			}
			GX.size=pb.SS.D;
		}
		
		// Option "non sentivity to rotation"				
		if (param.rotation>0) 
		{
			normPX=normL(PX,2);
			if (g!=s) normGX=normL(GX,2);
			if(normPX>0)
			{
				RotatePX=matrixRotation(PX);
			}
								
			if(g!= s && normGX>0)
			{
				RotateGX=matrixRotation(GX);							
			}			
		}
						
		// Exploitation tendencies
		switch (param.rotation)
		{
			default:				
			for (d = 0; d < pb.SS.D; d++)
			{	
				R.SW.V[s].v[d]=R.SW.V[s].v[d] +
				+	alea(0, param.c)*PX.v[d];
			}
			
			if (g!=s)
			{
				for (d = 0; d < pb.SS.D; d++)
				{	
					R.SW.V[s].v[d]=R.SW.V[s].v[d] 
					+	alea(0,param.c) * GX.v[d];			
				}
			}
			break;
							
			case 1:
			// First exploitation tendency
			if(normPX>0)
			{
				zz=param.c*normPX/sqrtD;
				aleaV=aleaVector(pb.SS.D, zz);							
				expt1=matrixVectProduct(RotatePX,aleaV);
	
				for (d = 0; d < pb.SS.D; d++)
				{
					R.SW.V[s].v[d]=R.SW.V[s].v[d]+expt1.v[d];
				}
			}
							
			// Second exploitation tendency
			if(g!=s && normGX>0)
			{						
				zz=param.c*normGX/sqrtD;
				aleaV=aleaVector(pb.SS.D, zz);								
				expt2=matrixVectProduct(RotateGX,aleaV);
					
				for (d = 0; d < pb.SS.D; d++)
				{
					R.SW.V[s].v[d]=R.SW.V[s].v[d]+expt2.v[d];
				}
			}
			break;						
		}
			
		// Update the position
		for (d = 0; d < pb.SS.D; d++)
		{	
			R.SW.X[s].x[d] = R.SW.X[s].x[d] + R.SW.V[s].v[d];			
		}
										
		if (R.nEval >= pb.evalMax) goto end;
								
		// --------------------------
		noEval = 1;
						
		// Quantisation
		R.SW.X[s] = quantis (R.SW.X[s], pb.SS);
	
		switch (param.clamping)
		{			
			case 0:	// No clamping AND no evaluation
			outside = 0;
						
			for (d = 0; d < pb.SS.D; d++)
			{			
				if (R.SW.X[s].x[d] < pb.SS.min[d] || R.SW.X[s].x[d] > pb.SS.max[d])
									outside++;				
			}
	
			if (outside == 0)	// If inside, the position is evaluated
			{		
				R.SW.X[s].f =
					perf (R.SW.X[s], pb.function, pb.SS);					
				R.nEval = R.nEval + 1;								
			}				
			break;
						
			case 1:	// Set to the bounds, and v to zero
			for (d = 0; d < pb.SS.D; d++)
			{	
				if (R.SW.X[s].x[d] < pb.SS.min[d])
				{	
					R.SW.X[s].x[d] = pb.SS.min[d];
					R.SW.V[s].v[d] = 0;
				}
					
				if (R.SW.X[s].x[d] > pb.SS.max[d])
				{			
					R.SW.X[s].x[d] = pb.SS.max[d];
					R.SW.V[s].v[d] = 0;			
				}				
			}
										
			R.SW.X[s].f =perf(R.SW.X[s],pb.function, pb.SS);
			R.nEval = R.nEval + 1;				
			break;			
		}			
						
			// ... update the best previous position
			if (R.SW.X[s].f < R.SW.P[s].f)	// Improvement
			{		
				R.SW.P[s] = R.SW.X[s];
					
				// ... update the best of the bests
				if (R.SW.P[s].f < R.SW.P[R.SW.best].f)
				{		
					R.SW.best = s;			
				}				
			}		
		}			// End of "for (s=0 ...  "	
			
		// Check if finished
		switch (param.stop)
		{
			default:			
			error = R.SW.P[R.SW.best].f;
			break;
			
			case 2:
			error=distanceL(R.SW.P[R.SW.best],pb.solution,2);
			break;
		}
		error= fabs(error - pb.epsilon);
		
		if (error < errorPrev)	// Improvement
		{		
			initLinks = 0;							
		}
		else			// No improvement
		{			
			initLinks = 1;	// Information links will be	reinitialized	
		}
						
		errorPrev = error;
	end:					
		switch (param.stop)
		{		
			case 0:
			case 2:				
			if (error > pb.epsilon && R.nEval < pb.evalMax)
					noStop = 0;	// Won't stop
			else
				noStop = 1;	// Will stop
			break;
					
			case 1:			
			if (R.nEval < pb.evalMax)
					noStop = 0;	// Won't stop
			else
					noStop = 1;	// Will stop
			break;		
		}
					
	} // End of "while nostop ...
				
	// printf( "\n and the winner is ... %i", R.SW.best );			
	// fprintf( f_stag, "\nEND" );
	R.error = error;
	return R;  
}

  
    // ===========================================================
double alea(double a, double b) 
/* Random real  number between a and b */
{
	double 	r;

	// Normally, RAND_MAX = 32767 = 2^15-1
	// Not very good. Replaced it by KISS
	
	r=(double)rand_kiss()/RAND_MAX_KISS;
	r= a+r*(b-a);

	return r;
} 
  
// ===========================================================
int alea_integer (int a, int b) 
{				// Integer random number in [a b]
  int ir;
  double r;
    
	r = alea (0, 1);
  ir = (int) (a + r * (b + 1 - a));
    
	if (ir > b)	ir = b;
    
	return ir;  
}
  
    // ===========================================================
double alea_normal (double mean, double std_dev) 
{ 
  /*
  Use the polar form of the Box-Muller transformation to obtain a pseudo
	random number from a Gaussian distribution 
  */ 
  double x1, x2, w, y1;  
      // double y2;
      
  do  
  {
		x1 = 2.0 * alea (0, 1) - 1.0;
		x2 = 2.0 * alea (0, 1) - 1.0;
		w = x1 * x1 + x2 * x2;     
	}
  while (w >= 1.0);
    
	w = sqrt (-2.0 * log (w) / w);
  y1 = x1 * w;
  // y2 = x2 * w;
  if(alea(0,1)<0.5) y1=-y1; 
  y1 = y1 * std_dev + mean;
  return y1;  
}

	
// =============================================================
struct velocity aleaVector(int D,double coeff)
{
	struct velocity V;
	int d;
	int i;
	int K=2; // 1 => uniform distribution in a hypercube
			// 2 => "triangle" distribution
	double rnd;
	
	V.size=D;
	
	for (d=0;d<D;d++)
	{								
		rnd=0;
		for (i=1;i<=K;i++) rnd=rnd+alea(0,1);		

		V.v[d]=rnd*coeff/K;
	}
	
	return V;
}
    
      // ===========================================================
double normL (struct velocity v,double L) 
{   // L-norm of a vector
	int d;     
	double n;
      
	n = 0;
      
	for (d = 0; d < v.size; d++)
		n = n + pow(fabs(v.v[d]),L);
      
	n = pow (n, 1/L);     
	return n;  
}
    
// ===========================================================
double distanceL (struct position x1, struct position x2,double L) 
{  // Distance between two positions
	// L = 2 => Euclidean	
	int d;     
	double n;
      
	n = 0;
      
	for (d = 0; d < x1.size; d++)
		n = n + pow (fabs(x1.x[d] - x2.x[d]), L);
      
	n = pow (n, 1/L);
  return n;    
}

//============================================================
struct matrix matrixProduct(struct matrix M1,struct matrix M2)
{
	// Two square matrices of same size
	struct matrix Product;
	int D;	
	int i,j,k;
	double sum;
	D=M1.size;
	for (i=0;i<D;i++)
	{
		for (j=0;j<D;j++)
		{			
			sum=0;
			for (k=0;k<D;k++)
			{
				sum=sum+M1.v[i][k]*M2.v[k][j];	
			}
			Product.v[i][j]=sum;			
		}	
	}
	Product.size=D;
	return Product;
}	


//=========================================================
struct matrix matrixRotation(struct velocity V)
{
	/*
		Define the matrice of the rotation V' => V
	where V'=(1,1,...1)*normV/sqrt(D)  (i.e. norm(V') = norm(V) )
	
	*/
	struct velocity B;
	int i,j,d, D;
	double normB,normV,normV2;
	//struct matrix reflex1; // Global variable
	struct matrix reflex2;
	struct matrix rotateV;
	double temp;
	
	D=V.size;
	normV=normL(V,2); normV2=normV*normV;
	reflex2.size=D;
	
	// Reflection relatively to the vector V'=(1,1, ...1)/sqrt(D)	
	// norm(V')=1
	// Has been computed just once  (global matrix reflex1)	
	
	//Define the "bisectrix" B of (V',V) as an unit vector
	B.size=D;
	temp=normV/sqrtD;
					
	for (d=0;d<D;d++)
	{
		B.v[d]=V.v[d]+temp;
	}
	normB=normL(B,2);
			
	if(normB>0)
	{
		for (d=0;d<D;d++)
		{
			B.v[d]=B.v[d]/normB;
		}
	}
					
	// Reflection relatively to B
	for (i=0;i<D;i++)
	{
		for (j=0;j<D;j++)
		{
			reflex2.v[i][j]=-2*B.v[i]*B.v[j];
		}
	}
							
	for (d=0;d<D;d++)
	{
		reflex2.v[d][d]=1+reflex2.v[d][d];
	}
						
	// Multiply the two reflections
	// => rotation				
	rotateV=matrixProduct(reflex2,reflex1);

	return rotateV;
	
}
//==========================================================
struct velocity	matrixVectProduct(struct matrix M,struct velocity V)
{
	struct velocity Vp;
	int d,j;
	int Dim;
	double sum;

	Dim=V.size;
	for (d=0;d<Dim;d++)
	{
		sum=0;
		for (j=0;j<Dim;j++)
		{
			sum=sum+M.v[d][j]*V.v[j];	
		}
		Vp.v[d]=sum;
	}	

	Vp.size=Dim;
	return Vp;
}

// ===========================================================
int sign (double x) 
{     
	if (x == 0)	return 0;
  	if (x < 0)	return -1;    
			return 1;   
}
    
// ===========================================================
struct position quantis (struct position x, struct SS SS) 
{     
	/*
	  Quantisatition of a position
	Only values like x+k*q (k integer) are admissible 
	 */ 
  int d;
  double qd;
  struct position quantx;

	quantx = x;     

	for (d = 0; d < x.size; d++)
	{
	    qd = SS.q.q[d];	
	  
		if (qd > zero)	// Note that qd can't be < 0
	  {     
			// TJ: due to the fact that we are using
			// AmiBroker step as qd it
			// does not require min/max scaling as in original SPSO 
			//qd = qd * (SS.max[d] - SS.min[d]) / 2;	      
			quantx.x[d] = qd * floor (0.5 + x.x[d] / qd);	    
		}
	}
	return quantx;    
}

// TJ:
// The perf() function calculates fitness
// The original is totally replaced by 
// our own function that just calls back AmiBroker Evaluate function
// that in turn performs actual backtest
// Since PSO is designed to find MINIMUMS of functions
// and our Evaluate() is supposed to return best (highest) results
// we need to return value with negative sign
// ===========================================================
double perf (struct position x, int function, struct SS SS) 
{				// Evaluate the fitness value for the particle of rank s   
	int d;

	extern struct OptimizeParams *g_pOptParams;
	extern double (*g_pfEvaluateFunc)( void * );
	extern void   *g_pOptimizerContext;

	for (d = 0; d < x.size; d++) 
	{
		g_pOptParams->Items[ d ].Current = x.x[ d ];
	}

	return - g_pfEvaluateFunc( g_pOptimizerContext ); // return with negative sign so max is found
}

// TJ:
// The problemDef() defines search space parameters
// Again the original is totally replaced by 
// our own function that takes parameter ranges and quantization
// from AmiBroker's own OptimizeParams structure
// and stores them in PSO-compatible way
struct problem problemDef(int functionCode)
{
	extern struct OptimizeParams *g_pOptParams;

	int d;
	struct problem pb;
	
	pb.function=functionCode;	
	pb.epsilon = 0.0001;	//0.0001 Acceptable error
	pb.objective = 0;       // Objective value

	pb.SS.D = g_pOptParams->Qty;	// Dimension

	// Define the solution point, for test
	// NEEDED when param.stop = 2 
	// i.e. when stop criterion is distance_to_solution < epsilon
	for (d=0; d < pb.SS.D;d++)
	{
		pb.solution.x[ d ] = 0; // the original had zero
		
		// however it may be 
		// g_pOptParams->Items[ d ].Min;  ???

		pb.SS.min[ d ] = g_pOptParams->Items[ d ].Min;
		pb.SS.max[ d ] = g_pOptParams->Items[ d ].Max;
		pb.SS.q.q[ d ] = g_pOptParams->Items[ d ].Step;	// Relative quantisation, in [0,1].   

		pb.evalMax = evalMax;// number of evaluations for each run

	}
		
	pb.SS.q.size = pb.SS.D;
	return pb;
}




/////////////////////////////////////////
//================================================== KISS
/*
 A good pseudo-random numbers generator

 The idea is to use simple, fast, individually promising
 generators to get a composite that will be fast, easy to code
 have a very long period and pass all the tests put to it.
 The three components of KISS are
        x(n)=a*x(n-1)+1 mod 2^32
        y(n)=y(n-1)(I+L^13)(I+R^17)(I+L^5),
        z(n)=2*z(n-1)+z(n-2) +carry mod 2^32
 The y's are a shift register sequence on 32bit binary vectors
 period 2^32-1;
 The z's are a simple multiply-with-carry sequence with period
 2^63+2^32-1.  The period of KISS is thus
      2^32*(2^32-1)*(2^63+2^32-1) > 2^127
*/

static ulong kiss_x = 1;
static ulong kiss_y = 2;
static ulong kiss_z = 4;
static ulong kiss_w = 8;
static ulong kiss_carry = 0;
static ulong kiss_k;
static ulong kiss_m;


void seed_rand_kiss(ulong seed) 
{
    kiss_x = seed | 1;
    kiss_y = seed | 2;
    kiss_z = seed | 4;
    kiss_w = seed | 8;
    kiss_carry = 0;
}


ulong rand_kiss() 
{
    kiss_x = kiss_x * 69069 + 1;
    kiss_y ^= kiss_y << 13;
    kiss_y ^= kiss_y >> 17;
    kiss_y ^= kiss_y << 5;
    kiss_k = (kiss_z >> 2) + (kiss_w >> 3) + (kiss_carry >> 2);
    kiss_m = kiss_w + kiss_w + kiss_z + kiss_carry;
    kiss_z = kiss_w;
    kiss_w = kiss_m;
    kiss_carry = kiss_k >> 30;
    return kiss_x + kiss_y + kiss_w;
} 
