/**
 ******************************************************************
 *
 * Module Name : Analysis.h
 *
 * Author/Date : C.B. Lirakis / 23-May-21
 *
 * Description : 
 *
 * Restrictions/Limitations :
 *
 * Change Descriptions :
 *
 * Classification : Unclassified
 *
 * References :
 *
 *******************************************************************
 */
#ifndef __ANALYSIS_hh_
#define __ANALYSIS_hh_
#  include "fftw3.h"

/// Analysis documentation here. 
class Analysis
{
public:
    /// Default Constructor
    Analysis(int32_t ArraySize, uint32_t NChan=1);
    /// Default destructor
    ~Analysis();
    /// Analysis function
    /*!
     * Description: 
     *   
     *
     * Arguments:
     *   
     *
     * Returns:
     *
     * Errors:
     *
     */
    void ScaleData(const int16_t *samples);

    void ComputeFFT(void);
    void DumpResults(void);


   inline void SetScale(double v) {fScale = v;};
   inline double GetScale(void) {return fScale;};
  
private:

    // Private Functions 
    void Hamming(uint32_t N);

    // Private Data
    fftw_plan fFFT;        /*! FFTW plan access. */
    // fft vectors used
    double *fIN;
    fftw_complex *fOUT;
    int32_t  fArraySize;   /*! FFT array sizes */
    uint32_t fNChannels;   /*! Number of channels in input data. */
    double   fScale;
    double  *fWindow; 

};
#endif
