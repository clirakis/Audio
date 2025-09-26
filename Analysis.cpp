/********************************************************************
 *
 * Module Name : Analysis.cpp
 *
 * Author/Date : C.B. Lirakis / 23-May-21
 *
 * Description : Generic Analysis
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 ********************************************************************/
// System includes.

#include <iostream>
using namespace std;
#include <string>
#include <cmath>
#include <sstream>
#include <ostream>
#include <fstream>
#include <cstring>
#include <cstdint>

// Local Includes.
#include "Analysis.hh"
#include "debug.h"
#include "fftw3.h"

/**
 ******************************************************************
 *
 * Function Name : Analysis constructor
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Analysis::Analysis (int32_t Array_size, uint32_t NChannel)
{
    SET_DEBUG_STACK;
    /*
     * This can be a user set variable, but for the moment
     * I'm setting it to be the value to convert from
     * counts to volts.
     * 1.12 volts peak-to-peak This can vary. Setting to a value of 1
     * for the moment. 
     */
    fScale     = 1.0;
    fArraySize = Array_size;
    fNChannels = NChannel;
    fWindow    = NULL;
    
    // If we got this far, might as well make an fftw plan.
    // Allocate the arrays for the computation.
    fIN  = (double *) fftw_malloc(Array_size * sizeof(double));
    fOUT = (fftw_complex *) fftw_malloc(Array_size * sizeof(fftw_complex));
    fFFT = fftw_plan_dft_r2c_1d(Array_size, fIN, fOUT, FFTW_ESTIMATE);
    //Hamming(Array_size);
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : Analysis destructor
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
Analysis::~Analysis (void)
{
    SET_DEBUG_STACK;
    // Free the FFTW plan
    fftw_destroy_plan(fFFT);

    // Free the working arrays
    fftw_free(fIN);
    fftw_free(fOUT);
    delete fWindow;
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : 
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
/**
 ******************************************************************
 *
 * Function Name : 
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Analysis::Hamming(uint32_t N)
{
  SET_DEBUG_STACK;
  double theta;
  double FN = (double) N;
  fWindow = new double[N];
  for (uint32_t i=0;i<N;i++)
  {
      theta = 2.0 * M_PI/FN * (double) i;
      fWindow[i] = 0.54 - 0.46 * cos(theta);
  }
  SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : 
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Analysis::ScaleData(const int16_t *samples)
{
    SET_DEBUG_STACK;
    /*
     * While the data may be stereo, assume we are inputting only
     * a single accelerometer on the TIP channel. (0)
     *
     * calculate the stride for this. 
     */
    memset(fIN, 0, fArraySize * sizeof(double));
    
    for (int32_t i=0; i<fArraySize; i++)
    {
	fIN[i] = fScale * (double) samples[i*fNChannels];
	if (fWindow)
	{
	    fIN[i] *= fWindow[i];
        }
    }
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : 
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Analysis::ComputeFFT(void)
{
    SET_DEBUG_STACK;
    memset (fOUT, 0, fArraySize * sizeof(fftw_complex));
    fftw_execute(fFFT);
    //DumpResults();
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : 
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void Analysis::DumpResults(void)
{
    SET_DEBUG_STACK;
    cout << "DUMP RESULTS" << endl;
    ofstream dat("results.dat", ios_base::out);
    /*
     * output format is pretty raw.
     * array of doubles orgainzied (Real, complex)  * array size
     */
    dat.write( (char *)fOUT, fArraySize * sizeof(fftw_complex));
    dat.close();
}
