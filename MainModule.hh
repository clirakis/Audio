/**
 ******************************************************************
 *
 * Module Name : MainModule.hh
 *
 * Author/Date : C.B. Lirakis / 11-Mar-25
 *
 * Description : Template for a main class
 *
 * Restrictions/Limitations : none
 *
 * Change Descriptions :
 * 26-Sep-25 CBL Added in include cstdint
 *
 * Classification : Unclassified
 *
 * References :
 *
 *
 *******************************************************************
 */
#ifndef __MAINMODULE_hh_
#define __MAINMODULE_hh_
#  include <cstdint>
#  include "CObject.hh" // Base class with all kinds of intermediate
#  include "filename.hh"
#  include "portaudio.h"

class Analysis;

/* Select sample format. */
#define PA_SAMPLE_TYPE  paInt16   // this is pretty important for buffer allocaiton. 
typedef short SAMPLE;             // aka, here, this is a 16 bit wide
#define SAMPLE_SILENCE  (0)       // defines no input. 

typedef struct
{
    uint32_t    frameIndex;  /* Index into sample array. */
    uint32_t    maxFrameIndex;
    uint32_t    nChannels;
    SAMPLE      *recordedSamples;
}
paTestData;


class MainModule : public CObject
{
public:
    /** 
     * Build on CObject error codes. 
     */
    enum {ENO_FILE=1, ECONFIG_READ_FAIL, ECONFIG_WRITE_FAIL, ENO_MEM,
	  ENO_DEVICE, ENO_STREAM, ENO_RECORD, ENO_LOGFILE};
    /**
     * Constructor recording accelerometer data. 
     * All inputs are in configuration file. 
     */
    MainModule(const char *ConfigFile, const char *Note=NULL);

    /**
     * for main module
     */
    ~MainModule(void);

    /*! Access the This pointer. */
    static MainModule* GetThis(void) {return fMainModule;};

    /**
     * Main Module DO
     * 
     */
    void Do(void);

    /**
     * Tell the program to stop. 
     */
    void Stop(void) {fRun=false;};

    /**
     * Control bits - control verbosity of output
     */
    static const unsigned int kVerboseBasic    = 0x0001;
    static const unsigned int kVerboseMSG      = 0x0002;
    static const unsigned int kVerboseFrame    = 0x0010;
    static const unsigned int kVerbosePosition = 0x0020;
    static const unsigned int kVerboseHexDump  = 0x0040;
    static const unsigned int kVerboseCharDump = 0x0080;
    static const unsigned int kVerboseMax      = 0x8000;
  
    // Public Functions
    bool Record(void);
    bool Play(void);
    void Stats(void);
    void EnumerateAvailable(void);

    friend ostream& operator<<(ostream &os, const MainModule &mm);
  
private:
    // Private Data
    bool fRun;
    /*!
     * Tool to manage the file name. 
     */
    FileName*    fn;          /*! File nameing utilities. */
    bool         fChangeFile; /*! Tell the system to change the file name. */
    ofstream    *fDataLog; 
  
    /*! 
     * Configuration file name. 
     */
    char   *fConfigFileName;

    /* Collection of configuration parameters. */
    bool   fLogging;       /*! Turn logging on. */

    /*!
     * Data for recording/playing back.
     */
    paTestData fData;             /*! Data collected, read or playback. */
    uint32_t   fNSamples;         /*! Derived value. */
    int32_t    fTotalFrames;      /*! Derived value  */
    int32_t    fSampleRate;       /*! Sample rate    */
    int32_t    fFramesPerBuffer;
    int32_t    fNSeconds;         /*! Number of seconds of data to record. */
    int32_t    fInput;            /*! Input device   */
    int32_t    fOutput;           /*! Output device  */
    bool       fDefault;          /*! Use default IO if set to true */
    int32_t    fVolume;           /*! Set input volume level -- Calibrate */

    Analysis  *fAnalysis;         /*! tools to analyze data. */
    char      *fNote;
  
    /* Private functions. ==============================  */

    /*!
     * Open the data logger. 
     */
    bool OpenLogFile(void);
    /*!
     * Write header in log file.
     */
    bool WriteLogHeader(void);
    /*!
     * Read the configuration file. 
     */
    bool ReadConfiguration(void);
    /*!
     * Write the configuration file. 
     */
    bool WriteConfiguration(void);

    /**
     * Is the input scan rate supported??
     */
    bool ScanRateSupported(void);

    bool SetVolume(void);

  
    /*! The static 'this' pointer. */
    static MainModule *fMainModule;

};
#endif
