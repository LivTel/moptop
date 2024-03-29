/* ccd_exposure.c
** Moptop PCO CCD library
*/
/**
 * Exposure routines to drive the PCO CMOS CCD.
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

/* data types */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
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
 * Routine to set the camera exposure triggering mode. 
 * @param external An integer boolean, if TRUE externally trigger, otherwise internally trigger exposures.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_Command_Set_Trigger_Mode
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Set_Trigger_Mode(int external)
{
	enum CCD_COMMAND_TRIGGER_MODE trigger_mode;
	
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Set_Trigger_Mode(%d): Started.",external);
#endif /* LOGGING */
	if(external)
		trigger_mode = CCD_COMMAND_TRIGGER_MODE_EXTERNAL;
	else
		trigger_mode = CCD_COMMAND_TRIGGER_MODE_INTERNAL;
	if(!CCD_Command_Set_Trigger_Mode(trigger_mode))
	{
		Exposure_Error_Number = 1;
		sprintf(Exposure_Error_String,"CCD_Exposure_Set_Trigger_Mode: CCD_Command_Set_Trigger_Mode failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Set_Trigger_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine used to determine whether the last trigger mode set was SOFTWARE (internal).
 * @return The routine returns TRUE if the last set trigger mode was CCD_COMMAND_TRIGGER_MODE_INTERNAL, FALSE otherwise.
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_Command_Get_Trigger_Mode
 */
int CCD_Exposure_Trigger_Mode_Is_Software(void)
{
	enum CCD_COMMAND_TRIGGER_MODE mode;
	
	if(!CCD_Command_Get_Trigger_Mode(&mode))
		return FALSE;
	return (mode == CCD_COMMAND_TRIGGER_MODE_INTERNAL);
}

/**
 * Routine used to determine whether the last trigger mode set was EXTERNAL.
 * @return The routine returns TRUE if the last set trigger mode was CCD_COMMAND_TRIGGER_MODE_EXTERNAL, FALSE otherwise.
 * @see #Exposure_Data
 * @see ccd_command.html#CCD_Command_Get_Trigger_Mode
 */
int CCD_Exposure_Trigger_Mode_Is_External(void)
{
	enum CCD_COMMAND_TRIGGER_MODE mode;
	
	if(!CCD_Command_Get_Trigger_Mode(&mode))
		return FALSE;
	return (mode == CCD_COMMAND_TRIGGER_MODE_EXTERNAL);
}

/**
 * Set the requested exposure length. We first set the timebase to CCD_COMMAND_TIMEBASE_US (microseconds),
 * and convert the exposure length from seconds to microseconds. We do this as a bias frame needs to use the minimum
 * exposure length, which for a PCO camera is less than 1 millisecond.
 * @param exposure_length_s The exposure length as an double in seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_US
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_US
 * @see ccd_command.html#CCD_COMMAND_TIMEBASE
 * @see ccd_command.html#CCD_Command_Set_Timebase
 * @see ccd_command.html#CCD_Command_Set_Delay_Exposure_Time
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Length_Set(double exposure_length_s)
{
	int exposure_length_us;
	
	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set(%.6f s): Started.",exposure_length_s);
#endif /* LOGGING */
	/* set exposure and delay timebase to microseconds */
	if(!CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US))
	{
		Exposure_Error_Number = 4;
		sprintf(Exposure_Error_String,"CCD_Exposure_Length_Set: "
			"CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US) failed.");
		return FALSE;
	}
	/* convert exposure length to microseconds. */
	exposure_length_us = (int)(exposure_length_s*((double)CCD_GENERAL_ONE_SECOND_US));
	/* set exposure length in microseconds */
	if(!CCD_Command_Set_Delay_Exposure_Time(0,exposure_length_us))
	{
		Exposure_Error_Number = 2;
		sprintf(Exposure_Error_String,
			"CCD_Exposure_Length_Set: CCD_Command_Set_Delay_Exposure_Time(0,%d) failed.",
			exposure_length_us);
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set: Exposure length set to %d us (%d ms).",
			       exposure_length_us,exposure_length_us/CCD_GENERAL_ONE_MILLISECOND_US);
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Set: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current exposure length in use by the PCO library/camera.
 * @param exposure_length_s The address of a double to store the returned exposure length in decimal seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see ccd_command.html#CCD_Command_Get_Exposure_Time
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Exposure_Length_Get(double *exposure_length_s)
{
	int delay_time,exposure_length_us;

	Exposure_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Get: Started.");
#endif /* LOGGING */
	if(exposure_length_s == NULL)
	{
		Exposure_Error_Number = 5;
		sprintf(Exposure_Error_String,"CCD_Exposure_Length_Get: exposure_length_s was NULL.");
		return FALSE;
	}
	/* CCD_Command_Get_Delay_Exposure_Time returns the delay_time and exposure_length in units
	** specified by CCD_Command_Set_Timebase. */
	/* set exposure and delay timebase to microseconds */
	if(!CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US))
	{
		Exposure_Error_Number = 6;
		sprintf(Exposure_Error_String,"CCD_Exposure_Length_Get: "
			"CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US) failed.");
		return FALSE;
	}
	if(!CCD_Command_Get_Delay_Exposure_Time(&delay_time,&exposure_length_us))
	{
		Exposure_Error_Number = 3;
		sprintf(Exposure_Error_String,"CCD_Exposure_Length_Get: CCD_Command_Get_Delay_Exposure_Time failed.");
		return FALSE;
	}
	/* convert retrieved time in microseconds to decimal seconds */
	(*exposure_length_s) = ((double)exposure_length_us)/((double)CCD_GENERAL_ONE_SECOND_US);
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Exposure_Length_Get: Returned exposure length %.6f s.",
			       (*exposure_length_s));
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
