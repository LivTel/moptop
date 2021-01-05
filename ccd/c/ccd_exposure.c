/* ccd_exposure.c
** Moptop Andor Zyla CCD library
*/
/**
 * Exposure routines to drive the Andor Zyla CMOS CCD.
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"
#include "ccd_exposure.h"

/* hash defines */
/**
 * The maximum length of enumerated value strings in Exposure_Data.
 */
#define EXPOSURE_ENUM_VALUE_STRING_LENGTH (64)

/* data types */
/**
 * Data type holding local data to ccd_exposure. This consists of the following:
 * <dl>
 * <dt>Trigger_Mode</dt> <dd>Enumerated string value of the currently configured trigger mode.
 *                       The string has length SETUP_ENUM_VALUE_STRING_LENGTH.
 *                       See possible values in Andor_SDK3_Manual.pdf, P52.</dd>
 * </dl>
 * @see #EXPOSURE_ENUM_VALUE_STRING_LENGTH
 */
struct Exposure_Struct
{
	char Trigger_Mode[EXPOSURE_ENUM_VALUE_STRING_LENGTH];
};


/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Exposure_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Trigger_Mode</dt> <dd>""</dd>
 * </dl>
 */
static struct Exposure_Struct Exposure_Data = 
{
	""
};

/**
 * Variable holding error code of last operation performed.
 */
static int Exposure_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Exposure_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to set the camera exposure triggering mode. The trigger_mode parameter is checked to ensure it is legal
 * (not NULL/not too long). CCD_Command_Set_Trigger_Mode is used to configure the camera. The trigger_mode parameter
 * is stored in Exposure_Data.Trigger_Mode so we can test what trigger mode is currently in use elsewhere.
 * @param trigger_mode Which mode to use. This should be one of: CCD_COMMAND_TRIGGER_MODE_SOFTWARE / 
 *        CCD_COMMAND_TRIGGER_MODE_EXTERNAL.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_Command_Set_Trigger_Mode
 * @see ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see ccd_command.html#CCD_COMMAND_TRIGGER_MODE_EXTERNAL
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Set_Trigger_Mode(char *trigger_mode)
{
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Set_Trigger_Mode: Started.");
#endif /* LOGGING */
	/* ensure trigger_mode parameter is OK */
	if(trigger_mode == NULL)
	{
		Exposure_Error_Number = 1;
		sprintf(Exposure_Error_String,"CCD_Exposure_Set_Trigger_Mode: trigger_mode was NULL.");
		return FALSE;
	}
	if(strlen(trigger_mode) >= EXPOSURE_ENUM_VALUE_STRING_LENGTH)
	{
		Exposure_Error_Number = 2;
		sprintf(Exposure_Error_String,"CCD_Exposure_Set_Trigger_Mode: trigger_mode was too long (%lu vs %d).",
			strlen(trigger_mode),EXPOSURE_ENUM_VALUE_STRING_LENGTH);
		return FALSE;
	}
	/* configure camera */
	if(!CCD_Command_Set_Trigger_Mode(trigger_mode))
	{
		Exposure_Error_Number = 3;
		sprintf(Exposure_Error_String,
			"CCD_Exposure_Set_Trigger_Mode: CCD_Command_Set_Trigger_Mode(%s) failed.",trigger_mode);
		return FALSE;
	}
	/* take a copy of trigger_mode used to configure the camera so we can query it later. */
	strcpy(Exposure_Data.Trigger_Mode,trigger_mode);
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Set_Trigger_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine used to determine whether the last trigger mode set was SOFTWARE.
 * @return The routine returns TRUE if the last set trigger mode was CCD_COMMAND_TRIGGER_MODE_SOFTWARE, FALSE otherwise.
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 */
int CCD_Exposure_Trigger_Mode_Is_Software(void)
{
	return strcmp(Exposure_Data.Trigger_Mode,CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
}

/**
 * Routine used to determine whether the last trigger mode set was EXTERNAL.
 * @return The routine returns TRUE if the last set trigger mode was CCD_COMMAND_TRIGGER_MODE_EXTERNAL, FALSE otherwise.
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_COMMAND_TRIGGER_MODE_EXTERNAL
 */
int CCD_Exposure_Trigger_Mode_Is_External(void)
{
	return strcmp(Exposure_Data.Trigger_Mode,CCD_COMMAND_TRIGGER_MODE_EXTERNAL);
}

/**
 * Set the requested exposure length. The actual exposure length may be changed by the Andor library.
 * @param exposure_length_s The exposure length as a double in seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_command.html#CCD_Command_Set_Exposure_Time
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Length_Set(double exposure_length_s)
{
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set: Exposure length set to %.6f s.",
			       exposure_length_s);
#endif /* LOGGING */
	if(!CCD_Command_Set_Exposure_Time(exposure_length_s))
	{
		Exposure_Error_Number = 4;
		sprintf(Exposure_Error_String,
			"CCD_Exposure_Length_Set: CCD_Command_Set_Exposure_Time(%.6f) failed.",exposure_length_s);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current exposure length in use by the Andor library/camera.
 * @param exposure_length_ms The address of an integer to store the returned exposure length in milliseconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_command.html#CCD_Command_Get_Exposure_Time
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Length_Get(int *exposure_length_ms)
{
	double exposure_length_s;

	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Get: Started.");
#endif /* LOGGING */
	if(exposure_length_ms == NULL)
	{
		Exposure_Error_Number = 5;
		sprintf(Exposure_Error_String,"CCD_Exposure_Length_Get: exposure_length_ms was NULL.");
		return FALSE;
	}
	if(!CCD_Command_Get_Exposure_Time(&exposure_length_s))
	{
		Exposure_Error_Number = 6;
		sprintf(Exposure_Error_String,
			"CCD_Exposure_Length_Get: CCD_Command_Get_Exposure_Time failed.");
		return FALSE;
	}
	(*exposure_length_ms) = (int)(exposure_length_s*((double)CCD_GENERAL_ONE_SECOND_MS));
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,
			       "CCD_Exposure_Length_Get: Retrieved exposure length is %.3f s / %d ms.",
			       exposure_length_s,(*exposure_length_ms));
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Get: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Exposure_Error_Number
 */
int CCD_Exposure_Get_Error_Number(void)
{
	return Exposure_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Exposure_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Exposure:Error(%d) : %s\n",time_string,
		Exposure_Error_Number,Exposure_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Exposure_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Exposure:Error(%d) : %s\n",time_string,
		Exposure_Error_Number,Exposure_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
