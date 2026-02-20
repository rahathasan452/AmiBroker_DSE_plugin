#include "stdafx.h"
#include "Plugin.h"

/*******************************************************
** TribesD.cpp file is based on TRIBES-D by Maurice Clerc
** Used with permission of the original author.
** 
** Original sources available from:
** http://www.particleswarm.info/Programs.html
** http://clerc.maurice.free.fr/pso/Tribes/TRIBES-D.zip
**
** Short summary about the method:
** http://www.particleswarm.info/Tribes_2006_Cooren.pdf
**
** Adapted for use in AmiBroker optimizer plugin architecture
** by Tomasz Janeczko, June 25, 2008
**
** Changes include:
** a) parameter passing "by value" changed to "by reference"
**      (to increase speed and decrease stack usaga)
** b) return values passed by reference instead (again to save stack space)
** c) some stack (auto) variables replaced by static variables to save stack
** d) replacement of problemRead and positionEval functions to integrate
**    with ADK
** e) few other adjustments
**
** Most changes were needed because original code run out of stack space
** 
** -- Tomasz Janeczko
** -- amibroker.com
*/
  
/*
     TRIBES-D, a fully adaptive parameter-free particle swarm optimiser
		 for real heterogeneous problems
                             -------------------
    last update           : 2008-02-01
    email                 : Maurice.Clerc@WriteMe.com

 ---- Why?
 In the complete TRIBES, there is an assumption
 saying that it is possible to define an Euclidean distance between
 two positions in the search space. In particular some points are
 chosen inside hyperspheres.
 
 However, for a lot of real problems this is meaningless. 
 For example, if one variable x1 is a cost, and another one x2 a weight,
 defining a distance by sqrt(x1*x1 + x2*x2) is completely arbitrary.
 Well, defining _any_ distance may be arbitrary. 
  
 TRIBES-D is "TRIBES without distance".
 Or, more precisely, without multi-dimensional distance.
 However this is not an algorithm for combinatorial problems.
 We still need to be able to compute the distance between 
 two points along one dimension, typically |x-y|.
 
 Of course, a drawback is that it is not very good on some artificial
 problems, particularly when the search space is a hypercube, and when
 the maximum number of fitness evaluations is small.
 I have added a few such problems from the CEC 2005 benchmark 
 so that you can nevertheless try TRIBES-D on them.
 
 Conversely, when the search space is not a hypercube, but a
 hyperparallelepid, and moreover when some dimensions are discrete,
 and the other ones continuous (heterogeneous problems)
 TRIBES-D may be pretty good.
 
 Note that in this version, the acceptable values for a variable
 can be not only an interval, but any given list of values.
 
 Also, compared to previous TRIBES versions, 
 it is far better for multiobjective problems.

 ---- Principles
- generate a swarm (typically one tribe one particle)
- at each time step
	- the informer of the "current" particle is the best particle
		of its tribe (the shaman)
	- if the particle is the shaman, select an informer at random amongst 
			the other shamans (mono-objective case) 
			or in the archive (multiobjective case).
			Note that the probability distribution to do that is
			not necessarily uniform
	- apply a strategy of move depending on the recent past
	
	From time to time (the delay may be different for each tribe):
		- check whether the tribe is bad or not
		- if bad, increase its size by generating a particle
		- if good and enough particles, remove the worst particle
	
	From time to time:
		- check whether the swarm is good or not
		- if bad, add a new tribe
		- if good, and if enough tribes, remove the worst tribe
			
---- About the strategies
	Depending on its recent past a particle may be good, neutral, or bad.
	So there are three strategies, one for each case.
	However, in order to add a kind of "natural selection" amongst strategies,
	a good particle keeps the same strategy (with a probability 0.5),
	no matter which one it is.

---- Tricks

- for a bad tribe there are two ways to increase diversity
	(see swarmAdapt()):
	- generate a completely new particle
	- modify the best position (the memory) of a existent one. 
		However this is done for just one dimension
		(the one with the smallest discrepancy over the whole tribe)

---- About multiobjective problems
In TRIBES-D, you can ask to solve simultaneously several problems.
First approach (when the codes functions are positive):
		the algorithm just tries to find the best compromise
		I am not sure it is very useful, but who knows?
		And anyway it costs nothing.
		
The second approach (negative code functions) is multiobjective. 
	The algorithm is looking for a Pareto front.
	It keeps up to date an archive of non dominated positions
	(a classical method).
	What is less clasical is that it keeps it over several runs
	(which can therefore be seen as manual"restarts")
	So, if the maximum number of fitness evaluations is FEmax, 
	you may try different strategies. For example just one run
	with FEmax, or 10 runs with FEmax/10. Actually the second method
	is usually better, although the algorithm is already pretty good
	when launching just one run.
	See fArchive.txt for the final result.
	
	For multibojective problems, the algorithm adaptively uses 
	two kinds of comparisons between positions:
	either classical dominance or an extension of the DWA 
	(Dynamic Weighted Aggregation) method.
	
	It also makes use of the Crowding Distance method, but not for
	all particles, only for the best one of each tribe.
	Unfortunately these method needs a parameter, in order to select
	the "guide". I tried here to replace this user defined parameter by 
	an adaptive one, depending on the number of tribes
	(see archiveCrowDistSelect()). 
	This is not completely satisfying, though.
		
---- About randomness
The standard rand() function in C is quite bad. That is why I use
for years a better one (KISS). 
Results are significantly different. Not necessarily better, for
the bad randomness of C implies a "granularity", and thanks to this
artifact, the solution may be found faster.

---- About a few useless little things
You may have noted that a few variables in some structures 
are not used (for example fPrevBest).

Similarly some features are noted "EXPERIMENT", or simply "commented"
There are here just for tests.
	
---- How to use TRIBES-D?
Data are read from the file problem.txt

In order to define your own problem, you have to modify this file and
positionEval().

Have fun, and keep me posted!
 	
*/
/* TO DO
- improve the local search, particularly for mono-objective problems
- non sensitivity to partMax (max. number of particles in a tribe)
- non sensitivity to tribMax
- non sensitivity to archiveMax (for multiobjective problems)
- automatic restart(s)
*/

/* TO TRY

(search the keyword "TO TRY" in the code)

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define ulong unsigned long // To generate pseudo-random numbers
#define RAND_MAX_KISS ((unsigned long) 4294967295)
	
#define DMax 	100 // Max. number of dimensions 
#define fMax   4+1 // Maximum number of objective functions
								// Note: number of "real" objective functions +1
#define initOptionsNb 5 // Number of options for initialisation of a particle
												// See particle()
#define partMax 	50 //Maximum number of particles in a tribe
#define rMax	500 // Maximum number of runs
#define tribMax	40 //Maximum number of tribes
#define valMax 11 // Maximum number of acceptable values on a dimension

// Specific to multiobjective
#define archiveMax 100 // Max. number of archived positions

// Structures
struct fitness    {int size;double f[fMax];};
struct problem		{int D;float min[DMax];float max[DMax];float dx[DMax];
										int fNb; int code[fMax];float objective[fMax];
										struct fitness errorMax;float evalMax;int repeat;
										int valSize[DMax];float val[DMax][valMax];};
					

struct position		{int size;double x[DMax];struct fitness f;};

struct particle		{int label;struct position x;struct position xBest;
										struct position xPrev;int strategy;
										struct fitness fBestPrev;int status;};
struct tribe		{int size;struct particle part[partMax];int best;
									struct fitness fBestPrev;int status;}; 
								// tribe size = number of particles
struct swarm		{int size; struct tribe trib[tribMax];struct position best;
									struct fitness fBestPrev;int status; 
									struct fitness fBestStag;};	
								// NOTE: swarm size = number of tribes

// Specific to multiobjective
struct archived {struct position x;double crowD;};	
struct distRank {double dist; int rank;};
									
//----------------------------------------- Subroutines
ulong	rand_kiss();
void	seed_rand_kiss(ulong seed);
//----

double alea(double a, double b);
double aleaGauss(double mean, double std_dev);
int aleaInteger(int a, int b);
									
void archiveCrowDist();
struct position archiveCrowDistSelect(int size);
void archiveDisplay();
struct fitness archiveFitnessVar();
void archiveLocalSearch(struct problem &pb);
void archiveSave(struct archived archiv[],int archiveNb,FILE *fArchive);
double archiveSpread();

int compareAdaptF(struct fitness *f1,int run,int fCompare);
static int compareCrowD (void const *a, void const *b); // For qsort
static int compareDR (void const *a,void const *b); // For qsort
static int compareFn (void const *a, void const *b); // For qsort
static int compareXY (void const *a, void const *b); // For qsort
	
void constraint(struct problem &pb,struct position &pos);

int fitnessCompare(struct fitness &f1,struct fitness &f2,int compareType);
void fitnessDisplay(struct fitness &f);
double fitnessDist(struct fitness &f1, struct fitness &f2);
double fitnessTot(struct fitness &f,int type);
	
double granul(double value,double granul);

double maxXY(double x, double y);
double minXY(double x,double y);

struct particle particleInit(struct problem &pb, int option,
				struct position &guide1,struct position &guide2, struct swarm &S);

void positionArchive(struct position &pos);
void positionDisplay(struct position &pos);
void positionEval(struct problem &pb, struct position &x, struct fitness &fit);
void positionEvalCEC2005(struct problem &pb, struct position &x, struct fitness &fit);
void	positionSave(FILE*fRun, struct position &pos,int run);
void positionUpdate(struct problem &pb,struct particle &par,
										struct particle &informer, struct position &pos);

void problemDisplay(struct problem &pb);																	
void problemRead(struct problem &pb);	
void problemSolve(struct problem &pb,int compareType,int run, struct swarm &S);
	
int sign(double x);

void swarmAdapt(struct problem &pb,struct swarm &S,int compareType, struct swarm &result);
void swarmDisplay(struct swarm &S);
void swarmInit(struct problem &pb,int compareType, struct swarm &S);
void swarmLocalSearch(struct problem &pb,struct swarm &S, struct swarm &result );
void swarmMove(struct problem &pb,struct swarm &S,
				int compareType,int run, struct swarm &result);
int swarmTotPart(struct swarm &S);
	
int	tribeBest(struct tribe &T,int compareType);
void tribeDisplay(struct tribe &T);
void tribeInit(struct problem &pb, int partNb,int compareType,
				struct swarm &S, struct tribe &result);
int tribeVarianceMin(struct tribe &T);
	
void wFAdapt(int fNb,int run);
	
//----------------------------------------- Global variables
double almostZero=1.e-14;//To be sure that, for example,
                         // associativity is true
double almostInfinite=1.e12; 
struct archived archiv[archiveMax+1];
int arch;
int archiveNb;
struct fitness archiveVar;

double e; // The famous Euler's constant (1731) ...
					// ... defined by Leibniz in 1683 (but he called it b)
double epsilon[fMax]; // For epsilon-dominance
float eval; // Number of fitness evaluations
struct fitness f1[rMax];
int fCompare;
int fn;
double fitMax[fMax]; // Maximum fitness value found during the process
double fitMin[fMax]; // Minimum "  "
int iter;
int iterLocalSearchNb;
int iterSwarmAdapt; // Number of iterations between two swarm adaptations
int iterSwarmStag;
int iterTribeAdapt[tribMax]; // The same, but for each tribe
int label; // label (integer number) of the last generated particle
int multiObj; // Flag for multiobjective problem
float o[DMax]; // Offset, in particular for CEC 2005 benchmark
int overSizeSwarm;// Nb of times the swarm tends to generate too many tribes
int overSizeTribe;// Nb of times a tribe tends to generate too many particles
double pi; // The Archimedes's constant
int restart=0;
int restartNb;
int verbose; // Read on the problem file. The higher it is, the more
							// verbose is the program
double wF[fMax+1]; // Dynamic penalties

/// 
/// Variables from main()

static struct position bestBest;	
static double errorMean[fMax];
float evalMean;
double errorTot;

int n;
static struct problem pb;
int run;
static double successRate[fMax];
static struct swarm S={0};

FILE *fRun; // To save the run
FILE *fArchive; // To save the Pareto front 


///////////// GLOBAL EXTERN () TribesD.h
int evalMax = 10000;
int runMax = 1;

//----------------------------------------- Main program
void init()
{
	//clock_t sclock = clock();


	seed_rand_kiss(1);
	e=exp((double)1);
	pi=acos((double)-1);
	overSizeSwarm=0;
	overSizeTribe=0;
		

	fRun=fopen("run.txt","w");
	fArchive=fopen("fArchive.txt","w");

	problemRead(pb); // pb passed as ref
	problemDisplay(pb);
	 
	errorTot=0;
	evalMean=0;
	archiveNb=0;

	// Kind of comparison, to begin (see fitnessCompare() )
	fCompare=0; // Note: not need to modify it for mono-objective
	 
	//Prepare final result
	 bestBest.size=pb.fNb;
	for(n=0;n<pb.fNb;n++) 
	{
		bestBest.f.f[n]=almostInfinite; // Arbitrary high value
		successRate[n]=0;
	}
	
	if(multiObj)
	{
		// Prepare epsilon-dominance
		// NOTE 1: to eliminate, it, just set epsilon[n] to zero
		// NOTE 2: as this setting is outside of the loop on runs
		//				each run will take advantage of the previous ones
		for(n=0;n<pb.fNb;n++) epsilon[n]=almostInfinite;
		for(n=pb.fNb;n<pb.fNb;n++)	epsilon[n]=0;
	}
	else
		for(n=0;n<pb.fNb;n++) epsilon[n]=0;
		
	restartNb=0;
		
	// Min and max fitness's
	for(n=0;n<pb.fNb;n++)
	{
		fitMin[n] = almostInfinite; // Very big value to begin
		fitMax[n] = 0;// -almostInfinite; // Very low value to begin
	}	
	
	run = 0;
}

//
// 	for (r=0;r<pb.repeat;r++)
//	{	

int optimize()
{	
	if(restart==0)
	{
		eval=0;// Init nb of fitness evaluations
					TRACE("\n\n RUN %i",run+1);
		TRACE("    fCompare %i",fCompare);	
	}

	restart=0;
	problemSolve(pb,fCompare,run, S); // S passed by ref
	
	f1[run]=S.best.f;

	if(run<pb.repeat-1)
	{
		// Define the next comparison method
		fCompare=compareAdaptF(f1,run,fCompare);
	}
	
	if(restart==1)
	{
		restartNb=restartNb+1;
		TRACE("\n Restart at eval %0.f",eval);
		run=run-1;
		goto end_r;
	}
		
	TRACE("\n %i restarts",restartNb);
	TRACE("\n Final swarm");
	swarmDisplay(S);
	TRACE("\n Final result after %i time steps, and %0.f evaluations:",iter,eval);
	fitnessDisplay(S.best.f);
	if(verbose>0) positionDisplay(S.best);

	// Save the result
	positionSave(fRun,S.best,run);

	// Prepare statistics
	for (n=0;n<pb.fNb;n++)
	{
		errorMean[n]=errorMean[n]+S.best.f.f[n];
		if(S.best.f.f[n]<pb.errorMax.f[n]) successRate[n]=successRate[n]+1;
	}

	// Save the best solution
	if (fitnessCompare(S.best.f,bestBest.f,2)==1) bestBest= S.best;

	evalMean=evalMean+eval;	
	end_r:;

	return ( ++run < pb.repeat );
}
	
void final()
{
	// Statistics
	for (n=0;n<pb.fNb;n++)
	{
		errorMean[n]=errorMean[n]/pb.repeat;
		successRate[n]=successRate[n]/pb.repeat;
	}
	evalMean=evalMean/pb.repeat;
	
	// Result
	TRACE("\n\n --- Result over %i runs",pb.repeat);
	TRACE("\n errorMean: ");
	for (n=0;n<pb.fNb;n++) TRACE("%f ",errorMean[n]);
	
	TRACE("\n Best result:  ");
	fitnessDisplay(bestBest.f);
	TRACE("\n on");
	positionDisplay(bestBest);

	TRACE("\n\n successRate: ");
	for (n=0;n<pb.fNb;n++) TRACE("%f ",successRate[n]);
	
	TRACE("\n evalMean %0.f",evalMean);

	if(overSizeTribe>0)
	{
		TRACE("\nWarning. Tribe size has been clamped %i times",overSizeTribe);
	}
	
	if(overSizeSwarm>0)
	{
		TRACE("\nWarning. Swarm size has been clamped %i times",overSizeSwarm);
	}
	
	if(multiObj)
		archiveSave(archiv,archiveNb,fArchive);
	
	fclose(fRun);
	fclose(fArchive);

	TRACE("\n END"); TRACE("\n");

	//TRACE("ETA: %g sec\n", 1.0*( clock() - sclock)/CLOCKS_PER_SEC );

	//getchar();
	return ;	
}

//================================================
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
//================================================
  double aleaGauss(double mean, double std_dev)
 {
 /*
     Use the polar form of the Box-Muller transformation to obtain
     a pseudo random number from a Gaussian distribution
 */
        double x1, x2, w, y1;
        //double  y2;

         do {
                 x1 = 2.0 * alea(0,1) - 1.0;
                 x2 = 2.0 * alea(0,1) - 1.0;
                 w = x1 * x1 + x2 * x2;
         } while ( w >= 1.0);

         w = sqrt( -2.0 * log( w ) / w );
         y1 = x1 * w;
        // y2 = x2 * w;
          y1=y1*std_dev+mean;
         return y1;
  }
//================================================
int aleaInteger(int min, int max)
/* Random integer number between min and max */
{
	double 	r;

	if(min>=max) return min;

	r=alea(min,max);
	return (int)floor(r+0.5);
}

//================================================
struct position archiveCrowDistSelect(int size)	
{
 	// Choose a random position, according to a non uniform distribution
	// of the crowding distance
	// the bigger the CD, the higher the probability
	// Also the bigger the number of tribes (i.e. "size") 
	// the "sharper" the distribution
	
	int n;
	double pr;
	// Compute the Crowding Distances
	archiveCrowDist();
	
	// Sort the archive by increasing Crowding Distance
	qsort(archiv,archiveNb,sizeof(archiv[0]),compareCrowD);
	
	// Choose at random according to a non uniform distribution:
	pr=2*log((double)(1+size));
	n=pow(alea(0,pow(archiveNb-1,pr)),1./pr);
	
	return archiv[n].x;
}
//================================================
void archiveCrowDist()
{
	// Compute the crowding distances in archive[n] (global variable)
	double dist;
	int fNb;
	int n;
	double max,min;
	
	fNb=archiv[0].x.f.size;
	
	for(n=0;n<archiveNb;n++) archiv[n].crowD=1; // Initialise
	
	for(fn=0;fn<fNb;fn++)
	{
		// Sort archive according to f[fn]. 
		// NOTE: fn is a global variable
		qsort(archiv,archiveNb,sizeof(archiv[0]),compareFn);

		for(n=0;n<archiveNb-1;n++)
		{
			// For each position find the distance to the nearest one
		if(n==0) min=fitMin[fn]; else min=archiv[n-1].x.f.f[fn];
		if(n==archiveNb-2) max=fitMax[fn]; else max=archiv[n+1].x.f.f[fn];
			
		dist=max-min;
		if(dist<epsilon[fn]) epsilon[fn]=dist; // Prepare epsilon-dominance
			// Contribution to the hypervolume
			archiv[n].crowD=archiv[n].crowD*dist;
		}
		
	}
}
//================================================
void archiveDisplay()
{
	int n;
	TRACE("\nArchive %i positions\n",archiveNb);
	
	for(n=0;n<archiveNb;n++)
	{
		positionDisplay(archiv[n].x);
		TRACE(" crowd %f",archiv[n].crowD);	
	}
}
//================================================
struct fitness archiveFitnessVar()
{
	// Variance of the archive for each fitness
	// More precisely return for each fitness g(var)
	// where g is a decreasing funtion of var:
	// The smaller the better
	double mean;
	int m,n;
	double var;
	struct fitness varF;
	double z;
	
	varF.size=archiv[0].x.f.size;

	for(n=0;n<varF.size;n++) 
	{
		mean=0;
		for(m=0;m<archiveNb;m++)
		{
			mean=mean+archiv[m].x.f.f[n];
		}
		mean=mean/archiveNb;
		
		var=0;
		for(m=0;m<archiveNb;m++)
		{
			z=archiv[m].x.f.f[n]-mean;
			var=var+z*z;
		}
		varF.f[n]=var/archiveNb;	
	}
	
	// Transform into something to minimise
	for(n=0;n<varF.size;n++) 
	{
		varF.f[n]=1./(1+varF.f[n]);
	}
	return varF;
}
//================================================
void archiveLocalSearch(struct problem &pb)
{
	/* Work with the global variables
	archiv[]
	archiveNb	
	arch
	
	Let fNb be the dimension of the fitness space.
	Loop:
		Define a simplex of dimension fNb
		Find a point "inside". Check if it is a good one
		or
		Find a point "outside". Check if it is a good one
	*/
	int d;
	struct distRank dR[archiveMax-1];
	int fNb;
	int m,n;
	int out;
	int r;
	struct position simplex[fMax+1];
	struct position xIn,xOut;
	
	fNb=archiv[0].x.f.size; // Dimension of the fitness space
	if(archiveNb<fNb+1) return;
	if(archiveNb<archiveMax) return;
	if(iterLocalSearchNb<archiveNb) return;
	
	TRACE("\nIter %i, Eval %0.f,  Local search from archive", iter,eval);
	iterLocalSearchNb=0;
	xIn.size=archiv[0].x.size;
	xOut.size=xIn.size;
		
	out=aleaInteger(0,1);
	for(n=0;n<archiveNb-fNb;n++)
	{	
		// Define a simplex
		simplex[0]=archiv[n].x; // First element	

		// Compute the distances to the others
		m=0;
		for(r=0;r<archiveNb;r++)
		{
			if(r==n) continue;
		
			dR[m].dist=fitnessDist(archiv[n].x.f,archiv[m].x.f);
			dR[m].rank=r;
				m=m+1;
		}
			
		// Find the fNb nearest ones in the archive
		// in order to complete the simplex
		fNb=archiv[0].x.f.size;
		qsort(dR,archiveNb-1,sizeof(dR[0]),compareDR);

		for(m=0;m<fNb;m++)
		{
			simplex[m+1]=archiv[dR[m].rank].x;
		}	

		// Define a new point
		//out=aleaInteger(0,1); // TO TRY
		//out=1;
		
//		TRACE("\n out %i",out);
		switch(out)
		{
			case 0:	// Inside the simplex
			for(d=0;d<xIn.size;d++)
			{
				xIn.x[d]=0;
				for(m=0;m<fNb+1;m++)
				{
					xIn.x[d]=xIn.x[d]+simplex[m].x[d];		
				}		
			}
			
			for(d=0;d<xIn.size;d++)
			{
				xIn.x[d]=xIn.x[d]/(fNb+1);
			}
			
			constraint(pb,xIn);
			positionEval(pb,xIn, xIn.f);
			positionArchive(xIn);
			break;
			
			case 1: // Outside
			for(d=0;d<xOut.size;d++)
			{
				xOut.x[d]=0;
				for(m=1;m<fNb+1;m++) // Partial simplex, dimension D-1
				{
					xOut.x[d]=xOut.x[d]+simplex[m].x[d];					
				}		
			}
			
			// Gravity center  
			for(d=0;d<xOut.size;d++)
			{
				xOut.x[d]=xOut.x[d]/fNb;
			}
			
			// Reflection of the first vertex
			for(d=0;d<xOut.size;d++)
			{
				xOut.x[d]=xOut.x[d] - (simplex[0].x[d]-xOut.x[d]);
			}
		
			constraint(pb,xOut);		
			positionEval(pb,xOut, xOut.f);
			positionArchive(xOut);
			break;
		}	

		if(arch==1) // If "good" position ..
		{
			// ... there is nevertheless a small probability
			// that the "curvature" is not the same for the next point.
			// and that it is interesting to try the other case
			if(alea(0,1)<1/(fNb+1)) out=1-out;
		}
		if(arch==0) // If "bad" position ...
		{
			// ... there is a high probability that 
			// the "curvature" is the same for the next point
		// and that is interesting to try the other case
			if(alea(0,1)<fNb/(fNb+1)) out=1-out;
		}
	}			
}
//================================================
void archiveSave(struct archived archiv[], int archiveNb,FILE *fArchive)
{
	int D;
	int fNb;
	int m;
	int n;
	TRACE("\n Save archive (%i positions)",archiveNb);
	D=archiv[0].x.size;
	fNb=archiv[0].x.f.size;
	TRACE("\n %i fitnesses, and %i coordinates",fNb,D);
	
	// Write the title
	for(n=0;n<fNb;n++)    fprintf(fArchive,"f%i ", n+1);
	for(n=0;n<D;n++)    fprintf(fArchive,"x%i ", n+1);
	fprintf(fArchive,"\n");	

	for(n=0;n<archiveNb;n++)
	{	
		for (m=0;m<fNb;m++)
		{
		 	fprintf(fArchive,"%f ",archiv[n].x.f.f[m]); // Fitnesses
		}

		for (m=0;m<D;m++)
		{
			fprintf(fArchive,"%f ",archiv[n].x.x[m]); // Position
		}
		fprintf(fArchive,"\n");
	}
}
//================================================
double archiveSpread()
{
	double d;
	double diversity1,diversity2;
	double dMin[archiveMax];
	double dMean1,dMean2;	
	int m,n;
	double noSpread,noSpread2;
	double z;
	
	if(archiveNb<2) return almostInfinite;
			
	// For each position, find the  nearest one
	// and the farest one, in the fitness space

	// Without the new position
	diversity1=0;
	for(n=0;n<archiveNb-1;n++)
	{
		dMin[n]=almostInfinite;
		for(m=0;m<archiveNb-1;m++)
		{
			if(m==n) continue;
			d=fitnessDist(archiv[n].x.f,archiv[m].x.f);
			if(d<dMin[n]) dMin[n]=d;
		}	
		diversity1=diversity1+dMin[n];
	}
	dMean1=diversity1/(archiveNb-1);
	
	// With the new position
	diversity2=0;
	for(n=0;n<archiveNb;n++)
	{
		dMin[n]=almostInfinite;
		for(m=0;m<archiveNb;m++)
		{
			if(m==n) continue;
			d=fitnessDist(archiv[n].x.f,archiv[m].x.f);
			if(d<dMin[n]) dMin[n]=d;
		}	
		diversity2=diversity2+dMin[n];
	}																
	dMean2=diversity2/archiveNb; 
	
	// "noSpread" estimation
	noSpread2=0;
	for(n=0;n<archiveNb;n++)
	{
		z=dMean2-dMin[n];
		noSpread2=noSpread2+z*z;
	}
	noSpread=sqrt(noSpread2);  // Initial value
	
	// Take distances (in the fitness space) into account
		// Distance between the new position and the first archived
	d=fitnessDist(archiv[0].x.f,archiv[archiveNb-1].x.f);
	
	for(n=1;n<archiveNb-1;n++) // Distance min to the others
	{
		z=fitnessDist(archiv[n].x.f,archiv[archiveNb-1].x.f);
		if(z<d) d=z;
	}
	if (d<dMean1) noSpread=noSpread+dMean1; // Penalty

	return noSpread;
}
//================================================
int compareAdaptF(struct fitness *f1, int runs,int fCompare)
{
	/*
	Modify the type of comparison, if multiobjective
	Possible methods:
	- adaptive weights
	- epsilon-dominance
	*/
	int compType;
	double dfMax,dfMin;
	int fNb;
	int fSame[fMax];
	double mean[fMax];
	int n;
	int same;
	int r;
	double var[fMax];
	double z;

	if (runs==0) return fCompare; // Not possible for the first run
	fNb=f1[0].size;
	if (fNb==1) return fCompare; // Nothing to do for mono-objective
	
	// Is the diversity big enough?
	// Mean 
	for (n=0;n<fNb;n++)
	{
		mean[n]=0;
		for (r=0;r<=runs;r++)
		{
			mean[n]=mean[n]+f1[r].f[n];
		}	
		mean[n]=mean[n]/(runs+1);		
	}

	// Variance
	for (n=0;n<fNb;n++)
	{
		var[n]=0;
		for(r=0;r<=runs;r++)
		{
			var[n]=pow(f1[r].f[n]-mean[n],2);
		}
		var[n]=sqrt(var[n]/(runs+1)); // Standard deviation
	}

	for(n=0;n<fNb;n++)
	{	
		dfMin=mean[n]-3*var[n]; 
		dfMax=mean[n]+3*var[n];
		if(dfMin<fitMin[n]) {fSame[n]=0; continue;}
		if(dfMax>fitMax[n]) {fSame[n]=0; continue;}

			// Fuzzy rule
			fSame[n]=0;
			z=alea(fitMin[n],mean[n]);
			if(z<dfMin) fSame[n]=1;
				
			z=alea(mean[n],fitMax[n]);
			if(z>dfMax) fSame[n]=1;	
	}

	same=0;
	for(n=0;n<fNb;n++)
	{
		if(fSame[n]==1) same=same+1;
	}
	
	// Either 0 or 1
	if(same==fNb) compType=1-fCompare; else compType=fCompare;
	return compType;
}

//================================================
static int compareCrowD (void const *a, void const *b)
{
	// For qsort (increasing order of Crowding Distance in the archive)
	 struct archived const *pa = (struct archived const *) a;
   struct archived const *pb = (struct archived const *) b;

	if(pa->crowD > pb->crowD) return 1;
		if(pa->crowD < pb->crowD) return -1;	
	 		return 0;
}

//================================================
static int compareDR (void const *a, void const *b)
{	
	// For qsort, increasing order of distance
	struct distRank const *pa = (struct distRank const *)a;
  struct distRank const *pb = (struct distRank const *)b;

	if(pa->dist > pb->dist) return 1;
		if(pa->dist < pb->dist) return -1;	
			return 0;	
}

//================================================
static int compareFn (void const *a, void const *b)
{	
	// For qsort (increasing order of f[fn] in the archive)
	// NOTE: needs the global variable fn
	struct archived const *pa = (struct archived const *)a;
  struct archived const *pb = (struct archived const *)b;

	if(pa->x.f.f[fn] > pb->x.f.f[fn]) return 1;
		if(pa->x.f.f[fn] < pb->x.f.f[fn]) return -1;
			return 0;
	
}
//================================================
static int compareXY (void const *a, void const *b)
{
	// For qsort (increasing order of any list of real numbers)
   double const *pa = (double const *)a;
   double const *pb = (double const *)b;

	if(*pa > *pb) return 1;
		if(*pa < *pb) return -1;
			return 0;
	
}
//================================================
void constraint(struct problem &pb,struct position &pos)
{
	int d;
	int n;
	//static struct position posG;
	int rank;

#define posG pos	
	
	// Granularity
	for(d=0;d<pb.D;d++)
	{
		if(pb.valSize[d]==0)
		{
			if(pb.dx[d]>0)
				posG.x[d]=granul(pos.x[d],pb.dx[d]);
		}
	}
	
	// Clamping
	for(d=0;d<pb.D;d++)
	{
		if(pb.valSize[d]==0) // Interval
		{
			if(posG.x[d]>pb.max[d]) posG.x[d]=pb.max[d];
			else
				if(posG.x[d]<pb.min[d]) posG.x[d]=pb.min[d];
		}
		else // List. Find the nearest one
		{
			rank=0;
			for(n=1;n<pb.valSize[d];n++)
			{
				if(fabs(pb.val[d][n]-posG.x[d])<
						fabs(pb.val[d][rank]-posG.x[d]))
					rank=n;				
			}
			posG.x[d]=pb.val[d][rank];
		}
	}

	// copy back
	//pos = posG;
#undef posG

	return;
}
//================================================
int fitnessCompare(struct fitness &f1,struct fitness &f2,int compareType)
{
	// 1 => f1 better than f2
	// 0 equivalent
	// -1 worse
	int better=0;
	int n;
	double wF1=0;
	double wF2=0;
	int worse=0;

	switch(compareType)
	{	
		case 1: // Using penalties		
			
		for(n=0;n<f1.size;n++)	
		{
			wF1=wF1+wF[n]*f1.f[n];
			wF2=wF2+wF[n]*f2.f[n];
		}
		
		if(wF1<wF2) return 1;
		if(wF2<wF1) return -1;
			return 0;

		default: //0  Epsilon-dominance		
		for(n=0;n<f1.size;n++)	
		{
			if(f1.f[n]<f2.f[n]+epsilon[n]) {better=better+1; continue;}
			if(f1.f[n]>f2.f[n]-epsilon[n]) {worse=worse+1;	continue;}
		}
	
		if(better>0 && worse==0) return 1;
		if(better==0 && worse>0) return -1;
		if(!multiObj) return 0;
			
			//Multiobjective
			//Difficult to decide. Use the "spread" criterion
			n=f1.size; // Contain the "noSpread" value
							// that should be as small as possible
			if(f1.f[n]<f2.f[n]) return 1;
				if(f1.f[n]>f2.f[n]) return -1;
					return 0;				

		case 2: // Pure dominance, for multiobjective 
						// Only to decide if a position has to be archived or not
		for(n=0;n<f1.size;n++)
		{
			if(f1.f[n]<f2.f[n]) {better=better+1; continue;}
			if(f1.f[n]>f2.f[n]) {worse=worse+1;	continue;}
		}
	
		if(better>0 && worse==0) return 1;
			if(better==0 && worse>0) return -1;	
				return 0;				
	}		
}	
//================================================
void fitnessDisplay(struct fitness &f)
{
	int i;
	TRACE("\n %i Fitness('s): ",f.size);

	for(i=0;i<f.size;i++)
	{
		TRACE(" %e",f.f[i]);	
	}
	if(multiObj) TRACE(" noSpread %f",f.f[f.size]);
}
//================================================
double fitnessDist(struct fitness &f1, struct fitness &f2)
{
	double dist=0;
	int n;
	double z;
	
	for(n=0;n<f1.size;n++)
	{
		z=f1.f[n]-f2.f[n];
		dist=dist+z*z;
	}
	
	return sqrt(dist);
}
//================================================
double fitnessTot(struct fitness &fit, int type)
{
	// Total fitness (weighted if multiobjective)
	double error;
	int n;
	
	if(fit.size<=1) return fit.f[0];
	
	switch (type)
	{
		default: // 0
		error=0;
		for (n=0; n<fit.size;n++)
		{
			error=error+fit.f[n]*fit.f[n];
		}
	
		error=sqrt(error);
		break;
	
		case 1: // Weighted (wF is a global variable)
		error=0;
		for (n=0; n<fit.size;n++)
		{
			error=error+wF[n]*fabs(fit.f[n]);
		}
		break;
	}
	
	return error;
}

//================================================
double granul(double value,double granul)
{
	// Modify a value according to a granularity 
	double xp;
	
	if (granul<almostZero) return value;// Pseudo-continuous
																			// (depending on the machine)

	if (value>=0) xp=granul*floor (value/granul+0.5); 
	else xp= (-granul*floor(-value/granul+0.5));

	return xp;
}
//================================================
double maxXY(double x,double y)
{
	if(x>y) return x; else return y;
}

//================================================
double minXY(double x,double y)
{
	if(x<y) return x; else return y;
}
//================================================
struct particle particleInit(struct problem &pb, int initOption,
	struct position &guide1,struct position &guide2,struct swarm &S)
{
	int d;
	double gamma;
	double mean;
	int n;
	int nSort;
	int pa;
	struct particle part;
	int rank;
	int tr;
	double xSort[2*tribMax*partMax];
	
	part.x.size=pb.D;
	switch(initOption)
	{
		default:	//0, Random position	
		for (d=0;d<pb.D;d++)
		{
				part.x.x[d]=alea(pb.min[d],pb.max[d]);						
		}
		break;
		
		case 1: // Guided
		for (d=0;d<pb.D;d++)
		{		
			if(pb.valSize[d]==0) // Interval
			{	
				gamma=fabs(guide1.x[d]-guide2.x[d]);
				part.x.x[d]=alea(guide1.x[d]-gamma,guide1.x[d]+gamma);
				//(TO TRY: aleaGauss instead)	
			}
			else // List
			{
			// WARNING: still purely random, not very satisfying
				n=aleaInteger(0,pb.valSize[d]-1);
				part.x.x[d]=pb.val[d][n];				
			}			
		}
		break;
	
		case 2: // On the corners
		for (d=0;d<pb.D;d++)
		{
				if(alea(0,1)<0.5) part.x.x[d]=pb.min[d];
				else part.x.x[d]=pb.max[d];
		}
		break;
		
		case 3: // Biggest empty hyperparallelepid (Binary Search)
			// TO TRY: for multiobjective, one may use the archive
			// instead of S as the list of known positions
		
		for (d=0;d<pb.D;d++)
		{	
			// List of known coordinates
			xSort[0]=pb.min[d];
			xSort[1]=pb.max[d];
			nSort=2;
			for(tr=0;tr<S.size;tr++)
			{
				if(S.trib[tr].size>0)
				{					
					for (pa=0;pa<S.trib[tr].size;pa++)
					{
						xSort[nSort]=S.trib[tr].part[pa].x.x[d];
						xSort[nSort+1]=S.trib[tr].part[pa].xBest.x[d];					
						nSort=nSort+2;
					}
				}
			}

			// Sort the coordinates
			if(nSort>1)
				qsort(xSort,nSort,sizeof(double),compareXY);
		
			// Find the biggest empty interval
			rank=1;
				
			for (n=2;n<nSort;n++)
			{
				if(xSort[n]-xSort[n-1]>xSort[rank]-xSort[rank-1])
				{
					rank=n;
				}
			}
			
			// Select a random position "centered" on the interval
			part.x.x[d]=alea(xSort[rank-1],xSort[rank]);				
		}				
		break;
		
		case 4: // Centered
			// EXPERIMENT
		for (d=0;d<pb.D;d++)
		{			
			mean=alea(pb.min[d],pb.max[d]);
			gamma=(1./3)*maxXY(pb.max[d]-mean,mean-pb.min[d]);
			part.x.x[d]=aleaGauss(mean,gamma);				
		}
		break;
	}

	constraint(pb,part.x); // Take constraints into account
		
	positionEval(pb, part.x, part.x.f); // Evaluate the position
	if(multiObj) positionArchive(part.x); // Archive (multiobjective)
		
	part.xPrev=part.x;
	part.xBest=part.x;
	part.fBestPrev=part.x.f;
	
	label=label+1;

	part.label=label;
	part.strategy=0; //TO TRY: aleaInteger(0,2);

	return part;
}		

//===============================================	
void positionArchive(struct position &pos)
{
	// Note: global variables 
	// archiv[], archiveNb
	
	int archiveStore;
	int fitC;
	int m,n;
	int dominanceType=2; // 2 => pure-dominance
		
	arch=1; // Global variable
	if(archiveNb>0) // It is not the first one. Check if dominated
	{
		for(n=0;n<archiveNb;n++)
		{
			fitC=fitnessCompare(archiv[n].x.f,pos.f,dominanceType);
			if(fitC==1) 
			{
				arch=0; // Dominated, don't keep it
				break;
			}
		}		
	}
	
	if(arch==1) 
	{		
		iterLocalSearchNb=iterLocalSearchNb+1; // Useful to decide
																					// when to perform local search
		// Remove the dominated positions
		if(archiveNb>1) 
		{
			for(n=0;n<archiveNb;n++)
			{
				fitC=fitnessCompare(pos.f,archiv[n].x.f,dominanceType);
				if(fitC==1)
				{
					if(n<archiveNb-1)
					{
						for(m=n;m<archiveNb-1;m++)
							archiv[m]=archiv[m+1];
					}
					archiveNb=archiveNb-1;				
				}
			}
		}
		
		// TO TRY: one may also remove one of two "too similar" positions
		
		// Store the position
		if(archiveNb<archiveMax) 
		{
			archiveStore=archiveNb;
		}
		else 
		{
			archiveNb=archiveMax;	
			// Find the most "crowded" archived position
			archiveStore=0;
			for(n=1;n<archiveNb;n++)
			{
				if(archiv[n].crowD<archiv[archiveStore].crowD)
					archiveStore=n;
			}
		}
			
		archiv[archiveStore].x=pos;
		if(archiveNb<archiveMax) archiveNb=archiveNb+1;
	}
}
//===============================================	
void positionDisplay(struct position &pos)
{
	int d;
	TRACE("\nx: ");
	
	for (d=0;d<pos.size;d++)
		TRACE("%.20f ",pos.x[d]);
	
	TRACE(" f: ");
	for(d=0;d<pos.f.size;d++)
		TRACE("%f ",pos.f.f[d]);
	if(multiObj) TRACE("noSpread %f",pos.f.f[pos.f.size]);
}

//===============================================		
void positionEval(struct problem &pb, struct position &x, struct fitness &fit)
{
	int d;

	extern struct OptimizeParams *g_pOptParams;
	extern double (*g_pfEvaluateFunc)( void * );
	extern void   *g_pOptimizerContext;

	for (d = 0; d < x.size; d++) 
	{
		g_pOptParams->Items[ d ].Current = x.x[ d ];
	}

	eval=eval+1;

	if( g_pOptParams->Step + 1 >= g_pOptParams->NumSteps )
	{
		g_pOptParams->NumSteps += 10;
	}

	double f = - g_pfEvaluateFunc( g_pOptimizerContext ); // return with negative sign so max is found

	fit.size=pb.fNb;
		
	for (n=0;n<pb.fNb;n++)
	{
		fit.f[n] = f + 1e11; // must be positive

		/**** ORIGINAL

		if( ! multiObj )
			fit.f[n] = fabs( f - pb.objective[n] );
		else 
			fit.f[n] = f;	// Multiobjective
		*/
	}		
		
	// Save the min and the max fitness ever found
	for (n=0;n<pb.fNb;n++)
	{
		fitMin[n]=minXY(fitMin[n],fit.f[n]);
		fitMax[n]=maxXY(fitMax[n],fit.f[n]);
	}
	
	// Additional "fitness" for multiobjective
	if(multiObj && archiveNb>0)
	{
		archiv[archiveNb].x.f=fit; // Temporary put in the archive
		archiveNb=archiveNb+1;

		fit.f[pb.fNb]=archiveSpread();
		
		archiveNb=archiveNb-1; // Virtually removed from the archive
	}	
		
	if(verbose>2) 
	{
		TRACE("\n In positionEval:");
		fitnessDisplay(fit);
		if (verbose>3) positionDisplay(x);
	}
	
	return;	
}

//===============================================	
void positionSave(FILE *fRun,struct position &pos,int run)
{
	// Save a position on a file
	
	int d;
	// Run
	fprintf(fRun,"%i %i %0.f ",run, iter, eval );
	
	// Fitness
	for(d=0;d<pos.f.size;d++)
		fprintf(fRun,"%f ",pos.f.f[d]);	
	
	// Position
	for (d=0;d<pos.size;d++)
		fprintf(fRun,"%f ",pos.x[d]);
	
	fprintf(fRun,"\n");
}

//================================================
void positionUpdate(struct problem &pb,struct particle &par,
				struct particle &informer, struct position &pos)
{
	// Here the particle MOVES
	double c1,c2;
	double c3=0;
	int d;
	int Dim;
	double error1=0;
	double error2=0;
	double gamma;
	double noise;
	struct position pos1,pos2;
	double posDelta[DMax];
	double radius;
	int strat[3];
	int type;

	
	/* Note that the following numerical values are not "parameters"
	  for they are derivated from mathematical analysis's,
		and, more important, they don't have to be tuned
		The user can perfectly ignore them.
	*/
	double w1=0.74; // => gaussian fifty-fifty 
	double w2=0.72; // < 1/(2*ln(2)) Just for tests
	double c=1.19; // < 0.5 + ln(2) Just for tests
	
	// Define the three strategies
	strat[0]=0; // For good particle
	strat[1]=1; // For neutral particle
	strat[2]=2; // For bad particle
	
	// Move
	pos.size=pb.D;
	Dim=informer.xBest.size; // May be zero if there is no informer
	
	switch(strat[par.strategy])
	{
		case -1: // Random
			// EXPERIMENT
		for(d=0;d<pb.D;d++)
		{
			pos.x[d]=alea(pb.min[d],pb.max[d]);			
		}
		break;
		
		
		default: //case 0: // Improved Bare bones
		for(d=0;d<Dim;d++)
		{
			gamma=w1*fabs(informer.xBest.x[d]-par.xBest.x[d]);
			pos.x[d]=aleaGauss(informer.xBest.x[d],gamma);		
		}

		break;
		
		case 1: // Pivot by dimension
		case 2: // Pivot by dimension + noise	
		if(multiObj) type=1;else type=0;

		if(Dim>0) // If there is an informer
		{
			error1=fitnessTot(par.xBest.f,type);
			error2=fitnessTot(informer.xBest.f,type);
	
			c3=error1+error2;
			if(c3>almostZero)
			{
				c1=error2/c3;
			}
			else 
			{
				c1=alea(0,1); 
			}
			c2=1-c1;
			
			for(d=0;d<Dim;d++)
			{	
				radius=fabs(par.xBest.x[d]-informer.xBest.x[d]);
				pos1.x[d]= alea(par.xBest.x[d]-radius,par.xBest.x[d]+radius);
				pos2.x[d]= alea(informer.xBest.x[d]-radius,informer.xBest.x[d]+radius);
				
				pos.x[d]=c1*pos1.x[d]+c2*pos2.x[d];		
			}
		}
		else // No other informer than itself
		{ 			
			for(d=0;d<Dim;d++)
			{		
				radius=maxXY(fabs(par.xBest.x[d]-pb.min[d]),
								fabs(par.xBest.x[d]-pb.max[d]));
				gamma=3*radius;
				pos.x[d]=aleaGauss(par.xBest.x[d],gamma);		
			}
		}
		
		if (par.strategy==1) break;
		// Add noise
		if(c3>almostZero)
		{
			noise=aleaGauss(0,(error1-error2)/c3);		
		}
		else
		noise=aleaGauss(0,1);
		
		for(d=0;d<pb.D;d++)
		{
			pos.x[d]=pos.x[d]+noise*(par.xBest.x[d]-pos.x[d]);		
		}
		break;	
		
		case 99:	// Standard PSO. Just for tests	
		for(d=0;d<Dim;d++)
		{
			posDelta[d]=w2*(par.x.x[d]-par.xPrev.x[d]);
			posDelta[d]=posDelta[d]+alea(0,c)*(par.xBest.x[d]-par.x.x[d])
								+ alea(0,c)*(informer.xBest.x[d]-par.x.x[d]);
			pos.x[d]=	par.xPrev.x[d]+posDelta[d];		
		}		
		break;	
	}

	//Granularity, Clamping, lists, etc.
	constraint(pb,pos);
	
	// Evaluate
	positionEval(pb,pos, pos.f);
	
	// Archive
	if(multiObj) positionArchive(pos);
}

//================================================
void problemDisplay(struct problem &pb)
{
	int d;
	int n;
	
	TRACE("\n Problem code(s):");
	for(n=0;n<pb.fNb;n++) TRACE(" %i",pb.code[n]);

	TRACE("\n Dimension %i", pb.D);
	TRACE("\n Search space:");
	for(d=0;d<pb.D;d++)
	{
		if(pb.valSize[d]==0)
			TRACE("\n [%f  %f], dx=%f",pb.min[d],pb.max[d],pb.dx[d]);
		else
		{
			TRACE("\nList: ");
			for(n=0;n<pb.valSize[d];n++)
				TRACE("%f ",pb.val[d][n]);				
		}
	}	

	TRACE("\n Objective  Admissible error");
	for (n=0;n<pb.fNb;n++) 
		TRACE("\n  %f          %f",pb.objective[n],pb.errorMax.f[n]);

	TRACE("\n Max. number of fitness evaluations: %.0f",pb.evalMax);
	TRACE("\n Number of runs: %i",pb.repeat);
}
//================================================
void problemRead(struct problem &pb)
{
	extern struct OptimizeParams *g_pOptParams;
	extern double (*g_pfEvaluateFunc)( void * );
	extern void   *g_pOptimizerContext;

	int d;
	int n;
	int OK;

	pb.D = g_pOptParams->Qty;

	for (d=0; d<pb.D; d++)
	{
		pb.min[ d ] = g_pOptParams->Items[ d ].Min;
		pb.max[ d ] = g_pOptParams->Items[ d ].Max;
		pb.dx[ d ]  = g_pOptParams->Items[ d ].Step;
		pb.valSize[d]=0;

		OK = 1;
	}		
	
	pb.fNb = 1; // ONE objective function

	for (n=0;n<pb.fNb;n++)
	{	
		pb.code[ n ] = 0; // function code (not used)
		pb.objective[ n ] = 0; // correct answer unknown
		pb.errorMax.f[ n ] = 0.0001; // max error - maybe add this as user-param????

	}
	pb.errorMax.size=pb.fNb;
	
	pb.evalMax = evalMax;
	pb.repeat = runMax;

	verbose = 1; // verbose evaluateion

	multiObj = 0; // NOT multi objective

	return;
}

//================================================
void problemSolve(struct problem &pb,int compareType,int run, struct swarm &S)
{
	int n;
	int stop=0;
	int tr;

	// Initials penalties (for multiobjective)
	for (n=0;n<pb.fNb;n++) wF[n]=1;

	// Prepare local search (for multiobjective)
	iterLocalSearchNb=0;
	
	//Initialisation of the swarm
	swarmInit( pb, compareType, S); // S passed by ref
	
	// Init of nb of iterations
	iter=0;
	iterSwarmAdapt=0; // Last iteration at which the swarm has been adapted
	
	for(tr=0;tr<S.size;tr++) iterTribeAdapt[tr]=0;
	
	// The algorithm itself
	while (stop==0)
	{
		for(tr=0;tr<S.size;tr++)
			iterTribeAdapt[tr]=iterTribeAdapt[tr]+1;
		
		iterSwarmAdapt=iterSwarmAdapt+1;	
		iterSwarmStag=iterSwarmStag+1;
		archiveVar=archiveFitnessVar();
		
		swarmMove(pb,S,compareType,run, S);
		swarmAdapt(pb,S,compareType, S);
		if(multiObj) archiveLocalSearch(pb);
		else 
			swarmLocalSearch(pb,S, S); // TO TRY
		iter=iter+1;

		//if(restart==1) {stop=1;continue;} // For future automatic restart
		if(eval>=pb.evalMax)  {stop=1; continue;}
		if(!multiObj && fitnessCompare(S.best.f,pb.errorMax,1)==1)
			{stop=1; continue;}
			
	}
	return;		
}
//================================================
int sign(double x)
{
	if(x>0) return 1;
	if(x<0) return -1;
	return 0;
}
//================================================
void swarmAdapt(struct problem &pb,struct swarm &S0,
	int compareType, struct swarm &result)
{
	int adaptSwarm;
	int addPart=0;
	int addTrib=0;
	int d;
	int disturbPart;
	static struct fitness f1,f2;
	int improvNb;
	int initOption;
	int linkNb;
	int pa;
	int partNb;
	int partWorst;
	double pr;
	int s;
	static struct swarm St;
	int status;
	int tr;
	int tribeNb;
	int trWorst;

	St=S0;	

//#define St S0

	tribeNb=St.size;
	//---------- (structural) adaptations of tribes
	// NOTE: for the moment, at each time step (iteration)

	for (tr=0;tr<St.size;tr++)
	{
		iterTribeAdapt[tr]=0; 

		// Status of the the tribe
		improvNb=0;
		for (pa=0;pa<St.trib[tr].size;pa++)
		{
			f1=St.trib[tr].part[pa].xPrev.f;
			f2=St.trib[tr].part[pa].x.f;

			if(fitnessCompare(f2,f1,compareType)==1) // If f2 is better than f1
				improvNb=improvNb+1;		
		}
		
		// Fuzzy rule	
		pr=improvNb/(float)St.trib[tr].size;

		if(alea(0,1)>pr)
			St.trib[tr].status=0; // Bad tribe
		else
		{
			St.trib[tr].status=1; // Good tribe			
		}	

		switch (St.trib[tr].status)
		{		
			case 0: // Bad tribe.
			/*
				The idea is to increase diversity.
			This is done by adding sometimes a completely new particle
			and by disturbing another one a bit, typically along
			just one dimension.
			*/			
			disturbPart=alea(0,1)<1-1./St.trib[tr].size;

			if(disturbPart) // Disturb a particle (not the best one)
			{
				if(St.trib[tr].size>1)
				{
					distPart:
					pa=aleaInteger(0,St.trib[tr].size-1);
					if(pa==St.trib[tr].best) goto distPart;
					
					d=aleaInteger(0,pb.D-1);								
					//d=tribeVarianceMin(St.trib[tr]); // EXPERIMENT
					St.trib[tr].part[pa].xBest.x[d]=
							alea(pb.min[d],pb.max[d]);
						
					// Should be MODIFIED. Too costly for just one modification					
					constraint(pb,St.trib[tr].part[pa].xBest);			
					positionEval(pb,  
					             St.trib[tr].part[pa].xBest, 
								 St.trib[tr].part[pa].xBest.f);
				}
			}
			
			// Add a new particle				
			partNb=St.trib[tr].size;
			if(partNb<partMax)
			{		
				if(multiObj) initOption=3;
				else initOption=aleaInteger(0,3);		
				
				St.trib[tr].part[partNb]=
				particleInit(pb,initOption,
				St.best,St.trib[tr].part[St.trib[tr].best].xBest,St); 
				
				St.trib[tr].size=partNb+1;
				addPart=addPart+1;
				// By extraordinary chance
				// this particle might be the best of the tribe
				f1=St.trib[tr].part[partNb].xBest.f;
				f2=St.trib[tr].part[St.trib[tr].best].xBest.f;
				if(fitnessCompare(f1,f2,compareType)==1) 
					St.trib[tr].best=partNb;
			}
			else
			{
				overSizeTribe=overSizeTribe+1;
				if(verbose>0)
				{
					TRACE("\n WARNING. Can't add particle");
					TRACE("\n  You may have to increase partMax (%i)",partMax);
				}
			}
			break;
				
			case 1: // Good tribe.;			
			if(St.trib[tr].size<2) break;	

			// Find the worst particle
			partWorst=0;
			for(pa=1;pa<St.trib[tr].size;pa++)
			{
				f1=St.trib[tr].part[partWorst].xBest.f;
				f2=St.trib[tr].part[pa].xBest.f;

				if(fitnessCompare(f2,f1,compareType)==-1) 
					partWorst=pa;				
			}
		
			// It might be also the best. In that case, don't remove it
			if(partWorst == St.trib[tr].best) break;
				
			// Remove it from the tribe	
			if(partWorst<St.trib[tr].size-1)
			{
				for(pa=partWorst;pa<St.trib[tr].size-1;pa++)
					St.trib[tr].part[pa]=St.trib[tr].part[pa+1];					
			}
			St.trib[tr].size=St.trib[tr].size-1;
			break;				
		}			
	}

	if(St.size>1)	
	{		
		linkNb=St.size*(St.size-1)/2;
		adaptSwarm=iterSwarmAdapt>=linkNb/2;
	}
	else
	{
		adaptSwarm=1==1; // Always true if there is just one tribe
	}

	// Reinitialise the previous best, and the iteration counter
	if(adaptSwarm) 
	{		
		St.fBestPrev=St.best.f;
		iterSwarmAdapt=0;
	
		// Status of the swarm
		if(fitnessCompare(St.best.f,St.fBestPrev,compareType)<=0)	
		{
			St.status=0; // Bad	swarm	
		}
		else 
		{
			St.status=1; // Good swarm	
		}
		
		switch (St.status)
		{
			case 0: // Bad swarm		
			// Add a new tribe
			if(St.size<tribMax)
			{
				tribeInit(pb,1,compareType, S0, St.trib[St.size]);				
			
				// By extraordinary chance,
				// it might contain the best particle of the swarm
				f1=St.trib[St.size].part[St.trib[St.size].best].xBest.f;
				f2=St.best.f;
			
				if(fitnessCompare(f1,f2,compareType)==1)
				{
					St.fBestPrev=St.best.f;
					St.best=St.trib[St.size].part[St.trib[St.size].best].xBest;
				}
				St.size=St.size+1;
				addTrib=addTrib+1;
			}
			else
			{
				overSizeSwarm=overSizeSwarm+1;
				if(verbose>0)
				{
					TRACE("\nWARNING. Can't add a tribe");
					TRACE("\n  You may have to increase tribMax (%i)",tribMax);
				}
			}
			break;
		
			case 1: // Good swarm		
			// Find the worst tribe
			if(St.size>1)
			{
				trWorst=0;
				for (tr=1;tr<St.size;tr++)
				{
					f1=St.trib[trWorst].part[St.trib[trWorst].best].xBest.f;
					f2=St.trib[tr].part[St.trib[tr].best].xBest.f;

					if(fitnessCompare(f2,f1,compareType)==-1) trWorst=tr;		
				}
			
				// Remove the worst tribe
				if(trWorst<St.size-1)
				{
					for (tr=trWorst;tr<St.size-1;tr++)
						St.trib[tr]=St.trib[tr+1];
				}
				St.size=St.size-1;
				if(verbose>3) TRACE("\n remove tribe %i",trWorst);				
			}
			break;
		}
	}
			
//-------- Adaptation of the  particles			
// Note: we don't immediately modify the strategy of a new particle

	for (tr=0;tr<tribeNb;tr++)  
	{
		for(pa=0;pa<St.trib[tr].size;pa++)
		{			
			f1=St.trib[tr].part[pa].xPrev.f;
			f2=St.trib[tr].part[pa].x.f;
			status=3*fitnessCompare(f2,f1,compareType);
			
			f1=St.trib[tr].part[pa].fBestPrev;
			f2=St.trib[tr].part[pa].xBest.f;
			status=status+fitnessCompare(f2,f1,compareType);
			
			if(status<=-2) St.trib[tr].part[pa].status=-1;
			else
				if(status>=3) St.trib[tr].part[pa].status=1;
					else St.trib[tr].part[pa].status=0;		
				
			switch(St.trib[tr].part[pa].status)
			{
				case -1: // Bad. Try another strategy
					anotherS:
					s=aleaInteger(0,2);
					if(s==St.trib[tr].part[pa].strategy) goto anotherS;
						St.trib[tr].part[pa].strategy=s;	
				break;
					
				case 0: // Normal
					St.trib[tr].part[pa].strategy=aleaInteger(1,2);				
				break;	
			
				case 1: // Good. Modify the strategy with a probability 0.5
					if(alea(0,1)<0.5) St.trib[tr].part[pa].strategy=0;
				break;				
			}	
		}	
	}
	
	// Display, if the swarm structure is modified
	if(verbose>=2)
	{
		if(addPart>0 || addTrib>0) 
		{
			swarmDisplay(St);	
		}
	}
	// Display in any case, if you want it
	if(verbose>=3) swarmDisplay(St);
		
	result = St;
//#undef St

	return;
}

//================================================
void swarmDisplay(struct swarm &S)
{
	int partNb=0;
	int tr;
	
	// Total number of particles
	for(tr=0;tr<S.size;tr++)
	{
		partNb=partNb+S.trib[tr].size;
	}
	
	if(partNb>1) TRACE("\nSwarm: %i particles",partNb);
	else TRACE("\nSwarm: %i particle",partNb);
	
	if(S.size>1) TRACE(" %i tribes",S.size); 
	else TRACE(" %i tribe",S.size); 

	if(verbose==1)
		for (tr=0;tr<S.size;tr++) TRACE(" %i",S.trib[tr].size);
			
	if(verbose>1)
	{
		for (tr=0;tr<S.size;tr++)
		{
			TRACE("\n tribe %i =>",tr+1);
			tribeDisplay(S.trib[tr]);
		}	
	}
}
//================================================
void swarmInit(struct problem &pb,int compareType, struct swarm &S)
{
	static struct fitness f1,f2;
	int partNb;
	int tr;
	int tribNb;

	if(verbose>0)
		TRACE("\n Initial swarm");
	
 	tribNb=1; // Initial number of tribes
	partNb=1; // Initial number of particles in each tribe
	
	for (tr=0;tr<tribNb;tr++) 
	{
		tribeInit(pb,partNb,compareType,S, S.trib[tr]);		
	}
		S.size=tribNb; // Number of tribes
									 // Warning: must be done AFTER tribeInit()
	
	// Find the best position of the swarm
	S.best=S.trib[0].part[S.trib[0].best].xBest;

	if(S.size>1)
	{
		for (tr=1;tr<S.size;tr++)
		{
			f1=S.trib[tr].part[S.trib[tr].best].xBest.f;
			f2=S.best.f;

			if(fitnessCompare(f1,f2,compareType)==1)
				S.best=S.trib[tr].part[S.trib[tr].best].xBest;	
		}
	}
 
	S.fBestPrev=S.best.f;
	S.fBestStag=S.best.f;
	S.status=0;

	if(verbose>0)
		swarmDisplay(S);
	return;
}
//================================================
void swarmLocalSearch(struct problem &pb,struct swarm &S, struct swarm &result )
{
	// EXPERIMENT
//	static struct swarm St;
	int d;
	double dist;
	static struct fitness f2;
	int m,mm;
	int out;
	int pa;
	int shaman;
	static struct position simplex[fMax+1];
	int tr;
	static struct position xNew;
	double w2=0.74;
	double z;
		
	int option=1;
	
#define St S
	//St=S;
	xNew.size=pb.D;
	
	switch(option)
	{
		case 0:
			
		for(tr=0;tr<St.size;tr++) // For each tribe
		{
			if(St.trib[tr].size<pb.D+1) continue; // If there is not enough particles
																							// do nothing
		
			if(verbose>0) 
				TRACE("\nIter %i, Eval %0.f,  Local search", iter,eval);
				
			// Define a simplex
			mm=St.trib[tr].best;
			for(m=0;m<pb.D+1;m++)
			{
				mm=mm+m; if(mm>St.trib[tr].size-1) mm=0;
				simplex[m]=St.trib[tr].part[mm].x;
			}	
	
			// Define a new point
				//out=aleaInteger(0,1); // TO TRY
			// out=1;
			out=0;
			
			switch(out)
			{
				case 0:	// Inside the simplex
				for(d=0;d<xNew.size;d++)
				{
					xNew.x[d]=0;
					for(m=0;m<pb.D+1;m++)
					{
						xNew.x[d]=xNew.x[d]+simplex[m].x[d]/simplex[m].f.f[0];		
					}		
				}
				
				for(d=0;d<xNew.size;d++)
				{
					xNew.x[d]=xNew.x[d]/(pb.D+1);
				}
				
				break;
				
				case 1: // Out
				for(d=0;d<xNew.size;d++)
				{
					xNew.x[d]=0;
					for(m=1;m<pb.D+1;m++) // Simplex dimension D
					{
						xNew.x[d]=xNew.x[d]+simplex[m].x[d];					
					}		
				}
				
				// Gravity center  
				for(d=0;d<xNew.size;d++)
				{
					xNew.x[d]=xNew.x[d]/pb.D;
				}
				
				// Reflection of the first vertex
				for(d=0;d<xNew.size;d++)
				{
					xNew.x[d]=xNew.x[d] - (simplex[0].x[d]-xNew.x[d]);
				}
				break;
			}
			
			constraint(pb,xNew);
			positionEval(pb,xNew, xNew.f);
				
			// Possibly update tribe best
			f2=St.trib[tr].part[St.trib[tr].best].xBest.f;
						
			if(fitnessCompare(xNew.f,f2,0)==1)
			{
				St.trib[tr].part[St.trib[tr].best].xBest=xNew;
				St.trib[tr].part[St.trib[tr].best].x=xNew;	
			}
			
			// Possibly update swarm best
			f2=St.best.f;
			
			if(fitnessCompare(xNew.f,f2,0)==1)
			{
				St.best=xNew;
			}	
		} // end "for(tr
		break;
		
		case 1: // "Around" the shaman
		for(tr=0;tr<St.size;tr++) // For each tribe
		{
			//if(St.trib[tr].size<pb.D+1) continue;
			if(St.trib[tr].size<2) continue;
				
			shaman=St.trib[tr].best;
			for(d=0;d<pb.D;d++) // for each dimension,
			{
				// find the nearest position
				dist=almostInfinite;
				for(pa=0;pa<St.trib[tr].size;pa++)
				{
					if(pa==shaman) continue;
						z=fabs(St.trib[tr].part[pa].xBest.x[d]-
							St.trib[tr].part[shaman].xBest.x[d]);
						if(z < dist) dist=z;						
				}
				
				// define a random intermediate coordinate
				z=St.trib[tr].part[shaman].xBest.x[d];
				//xNew.x[d]=z+aleaGauss(z,w2*dist);
				xNew.x[d]=z+(1-2*alea(0,1))*alea(0,dist);
			}
			
			constraint(pb,xNew);
			positionEval(pb,xNew, xNew.f);
			
			// Possibly update tribe best
			f2=St.trib[tr].part[shaman].xBest.f;
					
			if(fitnessCompare(xNew.f,f2,0)==1)
			{
				St.trib[tr].part[shaman].xBest=xNew;
				St.trib[tr].part[shaman].x=xNew;	
			}
		
			// Possibly update swarm best
			f2=St.best.f;
		
			if(fitnessCompare(xNew.f,f2,0)==1)
			{
				St.best=xNew;
			}
		}			
		
		break;
	} // end "switch(option 		
	
//	result = St;

#undef St

	return;
}
//================================================
void swarmMove(struct problem &pb, struct swarm &S,int compareType,
	                     int run, struct swarm &result )
{
	static struct fitness f1,f2;
	static struct particle informer={0}; // Initialised for my stupid compiler
	int pa;
	int sh=0;
	int shamanNb;
	int shBest;
	int shList[tribMax];
	//static struct swarm St;
	int tr;
	
#define St S
//	St=S;
	
	// Penalties (global variable wF)
	wFAdapt(pb.fNb,run);
	
	// First, memorise the best result of the whole swarm
	St.fBestPrev=S.best.f; 
	
	// Then indeed move 
	for (tr=0; tr<S.size;tr++) // For each tribe
	{	
		if(verbose>2)
		{
			TRACE("\n Best fitness of the tribe %i",tr);		
			fitnessDisplay(St.trib[tr].part[St.trib[tr].best].xBest.f);
		}			
		// Save the previous best result for each tribe
		S.trib[tr].fBestPrev=S.trib[tr].part[S.trib[tr].best].xBest.f;
		
		for(pa=0;pa<S.trib[tr].size;pa++) // For each particle ...
		{	
			// Save previous information
			St.trib[tr].part[pa].xPrev=St.trib[tr].part[pa].x;
			St.trib[tr].part[pa].fBestPrev=St.trib[tr].part[pa].xBest.f;
			
			if(pa!=St.trib[tr].best) // ... if it is not the shaman
			{
				informer=St.trib[tr].part[St.trib[tr].best];
			}				
			else // For the shaman, it is a bit more complicated
			{	
				if(multiObj) // Select an informer in the archive
				{
					informer.xBest=archiveCrowDistSelect(St.size);					
				}
				else // Mono-objective
					// Select an informer (attractor) in the swarm	
				 {
					if(St.trib[tr].size==1) // Just one particle =>
						                     // the informer is the shaman itself
					{
						informer=St.trib[tr].part[pa];
					}
					else // There are several tribes. Look for an external informer
					{
						// Build a random list of shamans
						shamanNb=aleaInteger(1,St.size); // Number of informer shamans
				
						for(sh=0;sh<shamanNb;sh++) // Define a random list
						{
							shList[sh]=aleaInteger(0,St.size-1);
						}
		
						// Find the best
						shBest=shList[0];
						if(shamanNb>1)
						{
							for(sh=1;sh<shamanNb;sh++)
							{
								f1=St.trib[shBest].part[St.trib[shBest].best].xBest.f;
								f2=St.trib[shList[sh]].part[St.trib[shList[sh]].best].xBest.f;
								if(fitnessCompare(f2,f1,compareType)==1) shBest=shList[sh];
							}
						}
		
						f1=St.trib[shBest].part[St.trib[shBest].best].xBest.f;
						f2=St.trib[tr].part[pa].xBest.f;
			
						if(fitnessCompare(f1,f2,compareType)) 
						{ // The informer is another shaman
							informer=St.trib[shBest].part[St.trib[shBest].best];
						}
						else // Not better than itself => no external informer
						{
							informer=St.trib[tr].part[pa];
						}	
					}						
				}
			
				if(verbose>2)
				{				
					TRACE("\n part. %i is informed by %i",
									St.trib[tr].part[pa].label,informer.label); 	
					fitnessDisplay(informer.xBest.f);	
				}
			}
				
			// Move according to the strategy of the particle
			positionUpdate(pb,St.trib[tr].part[pa], informer, St.trib[tr].part[pa].x);
		
			// Possibly update xBest of the particle		
			f1=St.trib[tr].part[pa].x.f;
			f2=St.trib[tr].part[pa].xBest.f;
	
			if(fitnessCompare(f1,f2,compareType)==1)
			{
				St.trib[tr].part[pa].xBest=St.trib[tr].part[pa].x;
				if(verbose>2)
				{
					TRACE("\n improvement => ");
					fitnessDisplay(f1);
				}
			}
			
			// Possibly update tribe best
			f2=St.trib[tr].part[St.trib[tr].best].xBest.f;
						
			if(fitnessCompare(f1,f2,compareType)==1)
			{
				St.trib[tr].best=pa;
				if(verbose>2)
				{
					TRACE("\n best particle is now %i",St.trib[tr].part[pa].label);
					fitnessDisplay(f1);			
				}	
			}
			
			// Possibly update swarm best
			f2=St.best.f;
			
			if(fitnessCompare(f1,f2,compareType)==1)
			{
				St.best=St.trib[tr].part[pa].x;
			}		
		}		
	}

//	result = St;
#undef St

	return;
}
//================================================
int swarmTotPart(struct swarm &S)
{
	// Compute the total number of particles
	int partNb=0;
	int tr;
	
	for (tr=0;tr<S.size;tr++)
		partNb=partNb+S.trib[tr].size;
	
	return partNb;
}
//================================================
int tribeBest(struct tribe &T,int compareType)
{
	// Find the best particle (shaman)
	int pa;
	int shaman=0;
	
	if(T.size>1)
	{
		for(pa=1;pa<T.size;pa++)
		{		
			if(fitnessCompare(T.part[pa].xBest.f,
						T.part[shaman].xBest.f,compareType)==1)
			{
				shaman=pa;						
			}
		}
	}
	return shaman;
}
//================================================
void tribeDisplay(struct tribe &T)
{
	int pa;
	TRACE("\n %i particles",T.size);
	
	TRACE("\n  Labels    :");
	for(pa=0;pa<T.size;pa++)
				TRACE(" %3i",T.part[pa].label);
	TRACE("\n  Strategies:");
	for(pa=0;pa<T.size;pa++)
				TRACE(" %3i",T.part[pa].strategy);
}

//================================================
void tribeInit(struct problem &pb, int partNb,
			int compareType,struct swarm &S, struct tribe &result)
{
	static struct position dumm={0};
	int initOption;
	int pa;
	static struct tribe newTribe={0};// Just because my compiler is too stupid
														// and would say it is uninitialized
	newTribe.best=0; // same comment
									
	
	if(verbose>3) TRACE("\n tribeInit");
		
	newTribe.size=minXY(partNb,partMax);
	newTribe.status=0;

	for(pa=0;pa<newTribe.size;pa++)
	{
		if(S.size==0) 
			initOption=0;
		else initOption=3;
			
		newTribe.part[pa]=particleInit(pb,initOption,dumm,dumm,S);
	}
	
	newTribe.best=tribeBest(newTribe,compareType);
	newTribe.fBestPrev=newTribe.part[newTribe.best].xBest.f;
	
	result = newTribe;
	return;
}
//================================================
int tribeVarianceMin(struct tribe &T)
{
	// EXPERIMENT
	// Return the dimension on which the variance of the tribe
	// is minimum 
	// Note: computed on the xBest positions
	int d;
	int dVarMin=-1;
	double meanD;
	int pa;
	int varD,varMin;
	double z;
	
	varMin=almostInfinite; // Arbitrary very big value
	
	for(d=0;d<T.part[0].x.size;d++)
	{
		meanD=0;
		for(pa=0;pa<T.size; pa++)
		{
			meanD=meanD+T.part[pa].xBest.x[d];	
		}	
		meanD=meanD/T.size;
		
		varD=0;
		for(pa=0;pa<T.size; pa++)
		{
			z=T.part[pa].xBest.x[d]-meanD;
			varD=varD+z*z;
		}	
		varD=varD/T.size;
		if(varD<varMin)dVarMin=d;
	}
	
	return dVarMin;
}

//================================================
void wFAdapt(int fNb,int run)
{
	// Modify the penalties
	// Global variables: pi, wF[]
	// NOTE: this is the most empirical part of the algorithm
	// It may certainly be largely improved
	int deb;
	int n;
	int m;

	m=aleaInteger(0,1);

	switch(m)
	{	
		case 0:
		for(n=0;n<fNb;n++) wF[n]=0; 
		m=aleaInteger(0,fNb-1); wF[m]=1;		
		break;
		
		case 1: // (Extended) Dynamic Weighted Aggregation
		deb=aleaInteger(0,fNb-1);
		for(n=0;n<fNb;n++)
		{
			m=n+deb; if(m>=fNb) m=fNb-m;
			wF[m]=fabs(sin(2*pi*(n+1)/fNb)); 
		}	
		break;
	}
}
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






















