/**
 ******************************************************************
 *
 * Module Name : 
 *
 * Author/Date : C.B. Lirakis / 19-Feb-25
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
// System includes.
#include <iostream>
#include <unistd.h>
using namespace std;

/// Local Includes.
#include "UserSignals.hh"
#include "Version.hh"
#include "MainModule.hh"
#include "CLogger.hh"

/** Control the verbosity of the program output via the bits shown. */
static unsigned int VerboseLevel = 0;

/** Pointer to the logger structure. */
static CLogger   *logger;
static bool ScanForDevices = false;
static char *Note = NULL;

/**
 ******************************************************************
 *
 * Function Name : Help
 *
 * Description : provides user with help if needed.
 *
 * Inputs : none
 *
 * Returns : none
 *
 * Error Conditions : none
 *
 *******************************************************************
 */
static void Help(void)
{
    SET_DEBUG_STACK;
    cout << "********************************************" << endl;
    cout << "* Test file for text Logging.              *" << endl;
    cout << "* Built on "<< __DATE__ << " " << __TIME__ << "*" << endl;
    cout << "* Available options are :                  *" << endl;
    cout << "*   -h help                                *" << endl;
    cout << "*   -n 'some note for the logfile'         *" << endl;
    cout << "*   -s Scan for devices                    *" << endl;
    cout << "*                                          *" << endl;
    cout << "********************************************" << endl;
}
/**
 ******************************************************************
 *
 * Function Name :  ProcessCommandLineArgs
 *
 * Description : Loop over all command line arguments
 *               and parse them into useful data.
 *
 * Inputs : command line arguments. 
 *
 * Returns : none
 *
 * Error Conditions : none
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
static void
ProcessCommandLineArgs(int argc, char **argv)
{
    int option;
    SET_DEBUG_STACK;
    do
    {
        option = getopt( argc, argv, "f:hHN:n:sSv");
        switch(option)
        {
        case 'f':
            break;
        case 'h':
        case 'H':
            Help();
        Terminate(0);
        break;
	case 'n':
	case 'N':
	  Note = strdup(optarg);
	  break;
	case 's':
	case 'S':
	  ScanForDevices = true;
	  break;
	case 'v':
	    VerboseLevel = atoi(optarg);
            break;
        }
    } while(option != -1);
}
/**
 ******************************************************************
 *
 * Function Name : Initialize
 *
 * Description : Initialze the process
 *               - Setup traceback utility
 *               - Connect all signals to route through the terminate 
 *                 method
 *               - Perform any user initialization
 *
 * Inputs : none
 *
 * Returns : true on success. 
 *
 * Error Conditions : depends mostly on user code
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
static bool Initialize(void)
{
    SET_DEBUG_STACK;
    char   msg[32];
    double version;

    SetSignals();
    // User initialization goes here. 
    snprintf(msg, sizeof(msg), "%d.%d",MAJOR_VERSION, MINOR_VERSION);
    version = atof( msg);
    logger = new CLogger("Accelerometer.log", "Accelerometer", version);
    logger->SetVerbose(VerboseLevel);

    return true;
}

/**
 ******************************************************************
 *
 * Function Name : main
 *
 * Description : It all starts here:
 *               - Process any command line arguments
 *               - Do any necessary initialization as a result of that
 *               - Do the operations
 *               - Terminate and cleanup
 *
 * Inputs : command line arguments
 *
 * Returns : exit code
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
int main(int argc, char **argv)
{
    ProcessCommandLineArgs(argc, argv);
    if (Initialize())
    {
        MainModule *pModule = new MainModule("Accelerometer.cfg", Note);

	if (pModule->Error() == 0)
	{
	    cout << *pModule;
	    if (ScanForDevices)
	    {
	        pModule->EnumerateAvailable();
	    }
	    else
	    {
	        pModule->Do();
	    }
	}

    }
    Terminate(0);
}
