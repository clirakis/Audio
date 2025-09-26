/**
 ******************************************************************
 *
 * Module Name : MainModule.cpp
 *
 * Author/Date : C.B. Lirakis / 11-Mar-25
 *
 * Description : Audo capture and analysis for Accelerometer data. 
 *
 * Restrictions/Limitations : none
 *
 * Change Descriptions : 
 *
 * Classification : Unclassified
 *
 * References : paex_record.c taken from 
 *              
 *	@ingroup examples_src
 *	@brief Record input into an array; Save array to a file; Playback recorded data.
 *	@author Phil Burk  http://www.softsynth.com
 *	https://github.com/EddieRingle/portaudio/blob/master/examples/paex_record.c
 *
 * dependences:
 * portaudio
 * libconfig++
 * fftw3
 *
 * NEXT STEP: clean up variable declaration
 *            how to continiously take data and feed the logfile and
 *              perform the fft. 
 *            clean up old files on a regular basis. 
 *
 * scan for devices:
 * https://portaudio.com/docs/v19-doxydocs/querying_devices.html
 * https://www.portaudio.com/docs/v19-doxydocs/tutorial_start.html
 *******************************************************************
 */  
// System includes.
#include <iostream>
using namespace std;

#include <string>
#include <cmath>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <libconfig.h++>
#include <ostream>
#include <sstream>
using namespace libconfig;

/// Local Includes.
#include "MainModule.hh"
#include "Analysis.hh"
#include "CLogger.hh"
#include "tools.h"
#include "debug.h"


MainModule* MainModule::fMainModule;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int recordCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    paTestData *data = (paTestData*)userData;
    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * data->nChannels];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if( inputBuffer == NULL )
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = SAMPLE_SILENCE;  /* left */
            if( data->nChannels == 2 ) *wptr++ = SAMPLE_SILENCE;  /* right */
        }
    }
    else
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( data->nChannels == 2 ) *wptr++ = *rptr++;  /* right */
        }
    }
    data->frameIndex += framesToCalc;
    return finished;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int playCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
    paTestData *data = (paTestData*)userData;
    SAMPLE *rptr = &data->recordedSamples[data->frameIndex * data->nChannels];
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    unsigned int i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        /* final buffer... */
        for( i=0; i<framesLeft; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( data->nChannels == 2 ) *wptr++ = *rptr++;  /* right */
        }
        for( ; i<framesPerBuffer; i++ )
        {
            *wptr++ = 0;  /* left */
            if( data->nChannels == 2 ) *wptr++ = 0;  /* right */
        }
        data->frameIndex += framesLeft;
        finished = paComplete;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( data->nChannels == 2 ) *wptr++ = *rptr++;  /* right */
        }
        data->frameIndex += framesPerBuffer;
        finished = paContinue;
    }
    return finished;
}

/**
 ******************************************************************
 *
 * Function Name : MainModule constructor
 *
 * Description : initialize CObject variables
 *
 * Inputs : currently none. 
 *
 * Returns : none
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
MainModule::MainModule(const char* ConfigFile, const char *Note) : CObject()
{
    CLogger *pLogger = CLogger::GetThis();
    PaError err = paNoError;
    ostringstream oss;

    /* Store the this pointer. */
    fMainModule = this;
    SetName("MainModule");
    SetError(); // No error.

    fRun = true;

    /* Default values if config file does not exist. */

    fLogging         = true;
    fSampleRate      = 44000;
    fFramesPerBuffer =   512;
    fNSeconds        =     5; // Seconds
    fInput           =     0; // Pa_GetDefaultInputDevice
    fOutput          =     0; // Default
    fDefault         = false;
    fVolume          =    50;
    fDataLog         = NULL;
    fNote            = NULL;
    
    if(!ConfigFile)
    {
	SetError(ENO_FILE,__LINE__);
	return;
    }

    fConfigFileName = strdup(ConfigFile);

    // Initalize PortAudio!
    err = Pa_Initialize();
    if( err != paNoError )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F',
			  "Initialization Error. ");
	SetError(ENO_MEM, __LINE__);
	SET_DEBUG_STACK;
        return;
    }

    if(!ReadConfiguration())
    {
	SetError(ECONFIG_READ_FAIL,__LINE__);
	return;
    }

    /* USER POST CONFIGURATION STUFF. */
    // Setup data array
    // Setup frame size. 
    fData.maxFrameIndex = fTotalFrames = fNSeconds * fSampleRate; 
    fData.frameIndex = 0;
    fNSamples = fTotalFrames * Pa_GetDeviceInfo( fInput )->maxInputChannels;
    
    /* From now on, recordedSamples is initialised. */
    fData.recordedSamples = new SAMPLE[fNSamples];
    
    if( fData.recordedSamples == NULL )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F',
			  "Could not allocate record array.");
	SetError(ENO_MEM, __LINE__);
	SET_DEBUG_STACK;
        return;
    }
    memset( fData.recordedSamples, 0, sizeof(SAMPLE)*fNSamples);

    fn       = NULL;
    if (fLogging)
    {
        fNote = strdup(Note);
	fn = new FileName("Accelerometer", "acc", One_Day);
	OpenLogFile();
    }
    fAnalysis = new Analysis(fNSamples);
    
    pLogger->Log("# MainModule constructed.\n");
    oss << *this;
    pLogger->Log(oss.str().data());
    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : MainModule Destructor
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
MainModule::~MainModule(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();

    // Close port audio. 
    Pa_Terminate();

    // Free up the data sample space. 
    if( fData.recordedSamples )       /* Sure it is NULL or valid. */
        delete[] fData.recordedSamples;
    
    // Write the configuration - maybe it changed.
    // in the instance that one did not exist, it will create
    // one with defaults. 
    if(!WriteConfiguration())
    {
	SetError(ECONFIG_WRITE_FAIL,__LINE__);
	Logger->LogError(__FILE__,__LINE__, 'W', 
			 "Failed to write config file.\n");
    }
    free(fConfigFileName);
    free(fNote);
    delete fAnalysis;

    // Close and delete logger.
    fDataLog->close();
    // This will close and flush the existing logfile.
    delete fDataLog;
    
    // Free the file naming tool. 
    if (fn)
      delete fn;
    
    // Make sure all file streams are closed
    Logger->Log("# MainModule closed.\n");
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
bool MainModule::Record(void)
{
    SET_DEBUG_STACK;

    PaStreamParameters  inputParameters;
    PaStream*           stream;
    PaError             err = paNoError;
    CLogger *pLogger = CLogger::GetThis();
    ClearError(__LINE__);

    pLogger->LogComment("Recording!\n");
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( fInput );

    inputParameters.device = fInput;
    inputParameters.channelCount = deviceInfo->maxInputChannels;
    fData.nChannels = inputParameters.channelCount;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              NULL,                  /* &outputParameters, */
              fSampleRate,
              fFramesPerBuffer,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              recordCallback,
              &fData );
    if( err != paNoError )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', "Could not open stream.");
        SetError(ENO_STREAM, __LINE__);
        SET_DEBUG_STACK;
        return false;
    }

    err = Pa_StartStream( stream );
    if( err != paNoError )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', "Could not record.");
        SetError(ENO_RECORD, __LINE__);
        SET_DEBUG_STACK;
        return false;
    }
    printf("\n=== Now recording!! Please speak into the microphone. ===\n"); fflush(stdout);

    while( (( err = Pa_IsStreamActive( stream ) ) == 1 ) && fRun)
    {
        Pa_Sleep(1000);
        printf("index = %d\n", fData.frameIndex ); fflush(stdout);
    }
    if( err < 0 )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', "Error with input stream.");
        SetError(ENO_RECORD, __LINE__);
        SET_DEBUG_STACK;
        return false;
    }

    err = Pa_CloseStream( stream );
    if( err != paNoError )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', Pa_GetErrorText(err));
        SetError(ENO_STREAM, __LINE__);
        SET_DEBUG_STACK;
        return false;
    }
    
    SET_DEBUG_STACK;
    return true;
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
bool MainModule::Play(void)
{
    SET_DEBUG_STACK;
    CLogger *pLogger = CLogger::GetThis();
    PaStreamParameters  outputParameters;
    PaStream*           stream;
    PaError err = paNoError;
    ClearError(__LINE__);
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( fOutput );

    /* Playback recorded data.  -------------------------------------------- */
    fData.frameIndex = 0;

    outputParameters.device = fOutput;
    outputParameters.channelCount = deviceInfo->maxOutputChannels;
    outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    printf("\n=== Now playing back. ===\n"); fflush(stdout);
    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              fSampleRate,
              fFramesPerBuffer,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              playCallback,
              &fData );
    if( err != paNoError )
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', Pa_GetErrorText(err));
        SetError(ENO_STREAM, __LINE__);
        SET_DEBUG_STACK;
        return false;
    }

    if( stream )
    {
        err = Pa_StartStream( stream );
        if( err != paNoError )
	{
	    pLogger->LogError(__FILE__, __LINE__, 'F', Pa_GetErrorText(err));
	    SetError(ENO_STREAM, __LINE__);
	    SET_DEBUG_STACK;
	    return false;
	}
        
        printf("Waiting for playback to finish.\n"); fflush(stdout);

        while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) Pa_Sleep(100);
        if( err < 0 ) return false;
        
        err = Pa_CloseStream( stream );
        if( err != paNoError )
	{
	    pLogger->LogError(__FILE__, __LINE__, 'F', Pa_GetErrorText(err));
	    SetError(ENO_STREAM, __LINE__);
	    SET_DEBUG_STACK;
	    return false;	  
	}
        
        pLogger->LogComment("Playback Done.\n");
    }

    SET_DEBUG_STACK;
    return true;
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
void MainModule::Stats(void)
{
    SET_DEBUG_STACK;
    SAMPLE  max, val;
    double  average;

     /* Measure maximum peak amplitude. */
    max     = 0;
    average = 0.0;
    for( uint32_t i=0; i<fNSamples; i++ )
    {
        val = fData.recordedSamples[i];
        if( val < 0 ) val = -val; /* ABS */
        if( val > max )
        {
            max = val;
        }
        average += val;
    }

    average = average / (double)fNSamples;

    printf("sample max amplitude = %d\n", max );
    printf("sample average = %lf\n", average );

    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : Do
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
void MainModule::Do(void)
{
    SET_DEBUG_STACK;
    CLogger *pLogger = CLogger::GetThis();
    const char *pSample;

    fRun = true;
 
    ClearError(__LINE__);
    pLogger->LogComment("DO!\n");
    
    if(!ScanRateSupported())
    {
        return;
    }

    if(Record() && fRun)
    {
        Stats();
        Play();
	if (fDataLog)
	{
	    pSample = (char *)fData.recordedSamples;
	    fDataLog->write(pSample, fNSamples*sizeof(SAMPLE));
	}
	fAnalysis->ScaleData(fData.recordedSamples);
	fAnalysis->ComputeFFT();
    }
    if (fLogging)
    {
        if (fn->ChangeNames())
        {
            /*
             * flush and close existing file
             * get a new unique filename
             * reset the timer
             * and go!
             *
             * Check to see that logging is enabled.
             */
            if(fDataLog)
            {
	        fDataLog->close();
                // This will close and flush the existing logfile.
	        delete fDataLog;
                fDataLog = NULL;
                // Now reopen
                OpenLogFile();
            }
        }
     }
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : OpenLogFile
 *
 * Description : Open and manage the log file
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool MainModule::OpenLogFile(void)
{
    SET_DEBUG_STACK;
    ClearError(__LINE__);
    // USER TO FILL IN.
    CLogger *pLogger = CLogger::GetThis();
    /* Give me a file name.  */
    const char* name = fn->GetUniqueName();
    fn->NewUpdateTime();
    SET_DEBUG_STACK;
    ClearError(__LINE__);
 
    /* Log that this was done in the local text log file. */
    time_t now;
    char   msg[64];
    SET_DEBUG_STACK;
    time(&now);
    strftime (msg, sizeof(msg), "%m-%d-%y %H:%M:%S", gmtime(&now));
    pLogger->Log("# changed file name %s at %s\n", name, msg);
    
    fChangeFile = false;
    fDataLog = new ofstream(name, ios::binary);
    if (!fDataLog)
    {
        pLogger->LogError(__FILE__,__LINE__, 'W',
			"Error opening data output stream");
        SetError(__LINE__, ENO_LOGFILE);
	return false;
    }
    WriteLogHeader();
    SET_DEBUG_STACK;
    return true;
}
/**
 ******************************************************************
 *
 * Function Name : ReadConfiguration
 *
 * Description : Open read the configuration file. 
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool MainModule::ReadConfiguration(void)
{
    SET_DEBUG_STACK;
    ClearError(__LINE__);
    CLogger *pLogger = CLogger::GetThis();
    Config *pCFG = new Config();

    /*
     * Open the configuragtion file. 
     */
    try{
	pCFG->readFile(fConfigFileName);
    }
    catch( const FileIOException &fioex)
    {
	pLogger->LogError(__FILE__,__LINE__,'F',
			  "I/O error while reading configuration file.\n");
	return false;
    }
    catch (const ParseException &pex)
    {
	pLogger->Log("# Parse error at: %s : %d - %s\n",
		     pex.getFile(), pex.getLine(), pex.getError());
	return false;
    }

    /*
     * Start at the top. 
     */
    const Setting& root = pCFG->getRoot();

    // USER TO FILL IN
    // Output a list of all books in the inventory.
    try
    {
	int    Debug;
	/*
	 * index into group MainModule
	 */
	const Setting &MM = root["MainModule"];
	MM.lookupValue("Logging",         fLogging);
	MM.lookupValue("Debug",           Debug);
	SetDebug(Debug);
	MM.lookupValue("SampleRate",      fSampleRate);
        MM.lookupValue("FramesPerBuffer", fFramesPerBuffer);
        MM.lookupValue("NSeconds",        fNSeconds);
	MM.lookupValue("InputDevice",     fInput);
	MM.lookupValue("OutputDevice",    fOutput);
	MM.lookupValue("DefaultIO",       fDefault);
	MM.lookupValue("Volume",          fVolume);
    }
    catch(const SettingNotFoundException &nfex)
    {
	// Ignore.
        pLogger->LogError(__FILE__, __LINE__,'W',"No config file.");
        return false;
    }

    delete pCFG;
    pCFG = 0;
    if (fOutput < 0)
    {
	fOutput = Pa_GetDefaultOutputDevice();
        pLogger->Log("# User selected default output device: %d\n",
		     fOutput);
    }
    if (fDefault)
    {
	fInput  = Pa_GetDefaultInputDevice();
	fOutput = Pa_GetDefaultOutputDevice();
        pLogger->Log("# User selected default device, input: %d, output %d\n",
		     fInput, fOutput);
    }
    SET_DEBUG_STACK;
    return true;
}

/**
 ******************************************************************
 *
 * Function Name : WriteConfigurationFile
 *
 * Description : Write out final configuration
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool MainModule::WriteConfiguration(void)
{
    SET_DEBUG_STACK;
    ClearError(__LINE__);
    CLogger *Logger = CLogger::GetThis();
    Config *pCFG = new Config();

    Setting &root = pCFG->getRoot();

    // USER TO FILL IN
    // Add some settings to the configuration.
    Setting &MM = root.add("MainModule", Setting::TypeGroup);
    MM.add("Debug",           Setting::TypeInt)     = 0;
    MM.add("Logging",         Setting::TypeBoolean) = true;
    MM.add("SampleRate",      Setting::TypeInt)     = fSampleRate;
    MM.add("FramesPerBuffer", Setting::TypeInt)     = fFramesPerBuffer;
    MM.add("NSeconds",        Setting::TypeInt)     = fNSeconds;
    MM.add("InputDevice",     Setting::TypeInt)     = fInput;
    MM.add("OutputDevice",    Setting::TypeInt)     = fOutput;
    MM.add("DefaultIO",       Setting::TypeBoolean) = fDefault;
    MM.add("Volume",          Setting::TypeInt)     = fVolume;
    // Write out the new configuration.
    try
    {
	pCFG->writeFile(fConfigFileName);
	Logger->Log("# New configuration successfully written to: %s\n",
		    fConfigFileName);

    }
    catch(const FileIOException &fioex)
    {
	Logger->Log("# I/O error while writing file: %s \n",
		    fConfigFileName);
	delete pCFG;
	return(false);
    }
    delete pCFG;
    SET_DEBUG_STACK;
    return true;
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
void MainModule::EnumerateAvailable(void)
{
    SET_DEBUG_STACK;
    //CLogger *pLogger = CLogger::GetThis();
    const   PaDeviceInfo *deviceInfo;
    uint32_t numDevices;
    ClearError(__LINE__);
    
    cout << "Default Input Device: " << Pa_GetDefaultInputDevice() << endl;
    cout << "Default Output Device: " << Pa_GetDefaultOutputDevice() << endl;
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        cout << "ERROR: Pa_CountDevices returned: " << numDevices  << endl;
        SetError(ENO_DEVICE,__LINE__);
        return;
    }
    cout << "Number of avaiable devices: " << numDevices << endl;
    for( uint32_t i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
	cout << i << ", " << deviceInfo->name
	     << ", Input Channels: " << deviceInfo->maxInputChannels
	     << ", Output Channels: " << deviceInfo->maxOutputChannels
	     << ", Default SR: " << deviceInfo->defaultSampleRate
	     << endl;
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
bool MainModule::ScanRateSupported(void)
{
    SET_DEBUG_STACK;
    CLogger *pLogger = CLogger::GetThis();
    bool rc = true;
    //    const   PaDeviceInfo *deviceInfo;
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    
    ClearError(__LINE__);
    // Get the data on the device selected.
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( fInput );
    cout << deviceInfo->maxInputChannels << endl;
    memset(&inputParameters, 0, sizeof(PaStreamParameters));
    inputParameters.channelCount     = deviceInfo->maxInputChannels;
    inputParameters.device           = fInput;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat     = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( fInput)->defaultLowInputLatency;

    deviceInfo = Pa_GetDeviceInfo( fOutput );
    memset(&outputParameters, 0, sizeof(PaStreamParameters));
    outputParameters.channelCount     = deviceInfo->maxOutputChannels;
    outputParameters.device           = fOutput;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat     = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( fOutput)->defaultLowOutputLatency;

    PaError err = Pa_IsFormatSupported( &inputParameters, &outputParameters,
					 (double) fSampleRate);
    if (err == paFormatIsSupported)
    {
        pLogger->Log("# Format supported!\n");
        rc = true;
    }
    else
    {
        pLogger->LogError(__FILE__, __LINE__, 'F', "Format not supported!");
        pLogger->Log("# PA error: %s\n", Pa_GetErrorText( err ));
        rc = false;
    }
    
    SET_DEBUG_STACK;
    return rc;
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
bool MainModule::SetVolume(void)
{
    SET_DEBUG_STACK;
    CLogger *pLogger = CLogger::GetThis();
    bool rc = true;

    //PaStreamParameters inputParameters;

    // Need to use PulseAudio for this. Later
    pLogger->LogError(__FILE__, __LINE__,'W',
		      "SetVolume function not implemented!");

    SET_DEBUG_STACK;
    return rc;
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
bool MainModule::WriteLogHeader(void)
{
    SET_DEBUG_STACK;
    static const uint32_t HEADER_SIZE = 256;
    ClearError(__LINE__);
    char zeros[HEADER_SIZE];
    ostringstream oss;
    bool rc = true;
    ClearError(__LINE__);

    oss << *this;
    int32_t n = oss.tellp();
    fDataLog->write(oss.str().data(), n);
    
    // Calculate the number of zeros to write.
    n = HEADER_SIZE - n;
    memset(zeros, 0, sizeof(zeros));
    fDataLog->write( zeros, n);
    SET_DEBUG_STACK;
    return rc;
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
ostream& operator<<(ostream& os, const MainModule &mm)
{
    SET_DEBUG_STACK;
    time_t now;
    char msg[64];
    char note[64];
    
    time(&now);
    strftime (msg, sizeof(msg), "%F %T", gmtime(&now));
    
    os << "Created: " << msg << endl
       << "Input: " << mm.fInput << endl
       << "Output: " << mm.fOutput << endl
       << "FramesPerBuffer: " << mm.fFramesPerBuffer << endl
       << "SampleRate: " << mm.fSampleRate << endl
       << "NChannels: " << mm.fData.nChannels << endl
       << "Volume: " << mm.fVolume << endl
       << "Note: " ;
    if (mm.fNote)
    {
        // Truncate note.
        memset (note, 0, sizeof(note));
        strncpy( note, mm.fNote, sizeof(note));
        os << note;
    }
    os << endl;
    SET_DEBUG_STACK;
    return os;
}
