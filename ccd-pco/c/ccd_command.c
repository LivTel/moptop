/* ccd_command.c
** Moptop PCO CCD library
*/
/**
 * Command wrapper around the PCO SDK library.
 * @author Chris Mottram
 * @version $Id$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"

/* data types */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Command_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Initialise the CCD library. 
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Initialise(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Finalise (finish using) the CCD library. 
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Get_Andor_Error_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Finalise(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Open a connection to the PCO camera and get a camera handle.
 * @return The routine returns TRUE on success and FALSE if it fails.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Open(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Open: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Open: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Close an open connection to the camera.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Close(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int CCD_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Command_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Command_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
