// Voronoi calculation code
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : February 27th 2008

#ifndef FACETS_CONSTANTS_HH
#define FACETS_CONSTANTS_HH

// These constants set the initial memory allocation for the Voronoi cell
const int initvertices=256;
const int initvertexorder=64;
const int init3vertices=256;
const int initnvertices=8;
const int initdubious=256;
const int initdeletesize=256;
const int initdeletesize2=256;
const int initfacetsize=32;

// If the initial memory is too small, the program dynamically allocates more.
// However, if the limits below are reached, then the program bails out.
const int maxvertices=16777216;
const int maxvertexorder=2048;
const int maxnvertices=16777216;
const int maxdubious=16777216;
const int maxdeletesize=16777216;
const int maxdeletesize2=16777216;
const int maxparticlemem=16777216;

// This sets the numerical tolerance. Below these values, the plane cutting
// snaps to existing vertices rather than creating new ones.
//typedef double f_point;
//const f_point tolerance=1e-10;
//const f_point tolerance2=2e-10;
typedef float f_point;
const f_point tolerance=1e-5;
const f_point tolerance2=2e-5;

#endif
