/* ccd_setup.c
** Moptop Andor Zyla CCD library
** $Header$
*/
/**
 * Routines to setup and configure the CCD.
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
#include <stdarg.h>
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"
#include "ccd_setup.h"

/* hash defines */
/**
 * The maximum length of enumerated value strings in Setup_Data.
 * The longest SensorReadoutMode value is 25 characters long..
 * The longest SimplePreAmpGainControl value is 40 characters long.
 */
#define SETUP_ENUM_VALUE_STRING_LENGTH (64)

/* data types */
/**
 * Data type holding local data to ccd_command. This consists of the following:
 * <dl>
 * <dt>Sensor_Readout_Mode</dt> <dd>Enumerated string that configures the direction in which the 
 *                              sensor will be read out. The string has length SETUP_ENUM_VALUE_STRING_LENGTH.</dd>
 * <dt>Pre_Amp_Gain_Control</dt> <dd>Enumerated value string that configures sensitivity and dynamic range using
 *                         the SimplePreAmpGainControl feature.The string has length SETUP_ENUM_VALUE_STRING_LENGTH.
 *                         See the Andor SDK manual Andor_SDK3_Manual.pdf P40 for possible values.</dd>
 * <dt>diddly</dt> <dd></dd>
 * </dl>
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 */
struct Setup_Struct
{
	char Sensor_Readout_Mode[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Pre_Amp_Gain_Control[SETUP_ENUM_VALUE_STRING_LENGTH];
};


/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Setup_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Sensor_Readout_Mode</dt> <dd>CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS</dd>
 * <dt>Pre_Amp_Gain_Control</dt> <dd>CCD_COMMAND_AMP_GAIN_16BIT_LOW</dd>
 * </dl>
 * @see ccd_command.html#CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS
 * @see ccd_command.html#CCD_COMMAND_AMP_GAIN_16BIT_LOW
 */
static struct Setup_Struct Setup_Data = 
{
	CCD_COMMAND_SENSOR_READOUT_MODE_OUTSIDE_IN_SIMULTANEOUS,
	CCD_COMMAND_AMP_GAIN_16BIT_LOW
};

/**
 * Variable holding error code of last operation performed.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Setup_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Do the initial setup for an Andor Zyla camera.
 * <ul>
 * <li>We initialise the libraries used using CCD_Command_Initialise.
 * <li>We open a connection to the CCD camera using CCD_Command_Open.
 * <li>We turn on sensor cooling using CCD_Command_Set_Sensor_Cooling.
 * <li>We turn on frame metadata using CCD_Command_Set_Metadata_Enable.
 * <li>We turn on frame timestamp metadata using CCD_Command_Set_Metadata_Timestamp.
 * <li>We turn off the spurious noise filter using CCD_Command_Set_Spurious_Noise_Filter.
 * <li>We turn off the static blemish correction using CCD_Command_Set_Static_Blemish_Correction.
 * <li>We turn on rolling shutter global clear readout mode using CCD_Command_Set_Rolling_Shutter_Global_Clear.
 * <li>We configure the direction in which the sensor will be read out, usingCCD_Command_Set_Sensor_Readout_Mode
 *     with the value stored in Setup_Data.Sensor_Readout_Mode.
 * <li>We configure the sensitivity and dynamic range using CCD_Command_Set_Simple_Pre_Amp_Gain_Control with the 
 *     value stored in Setup_Data.Pre_Amp_Gain_Control.
 * <ul>
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Initialise
 * @see ccd_command.html#CCD_Command_Open
 * @see ccd_command.html#CCD_Command_Set_Sensor_Cooling
 * @see ccd_command.html#CCD_Command_Set_Metadata_Enable
 * @see ccd_command.html#CCD_Command_Set_Metadata_Timestamp
 * @see ccd_command.html#CCD_Command_Set_Spurious_Noise_Filter
 * @see ccd_command.html#CCD_Command_Set_Static_Blemish_Correction
 * @see ccd_command.html#CCD_Command_Set_Rolling_Shutter_Global_Clear
 * @see ccd_command.html#CCD_Command_Set_Sensor_Readout_Mode
 * @see ccd_command.html#CCD_Command_Set_Simple_Pre_Amp_Gain_Control
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Setup_Startup(void)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Started.");
#endif /* LOGGING */
	/* initialise the Andor libraries */
	if(!CCD_Command_Initialise())
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise failed.");
		return FALSE;
	}
	/* open a connection to the CCD camera */
	if(!CCD_Command_Open())
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Open failed.");
		return FALSE;
	}
	/* initial configuration of the camera */
	/* turn cooling on */
	if(!CCD_Command_Set_Sensor_Cooling(TRUE))
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Sensor_Cooling(TRUE) failed.");
		return FALSE;
	}
	/* turn on frame metadata */
	if(!CCD_Command_Set_Metadata_Enable(TRUE))
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Metadata_Enable(TRUE) failed.");
		return FALSE;
	}
	/* turn on frame timestamp metadata */
	if(!CCD_Command_Set_Metadata_Timestamp(TRUE))
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Metadata_Timestamp(TRUE) failed.");
		return FALSE;
	}
	/* turn off the spurious noise filter */
	if(!CCD_Command_Set_Spurious_Noise_Filter(FALSE))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Spurious_Noise_Filter(FALSE) failed.");
		return FALSE;
	}
	/* turn off the static blemish correction */
	if(!CCD_Command_Set_Static_Blemish_Correction(FALSE))
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Static_Blemish_Correction(FALSE) failed.");
		return FALSE;
	}
	/* turn on the rolling shutter global clear readout mode */
	if(!CCD_Command_Set_Rolling_Shutter_Global_Clear(TRUE))
	{
		Setup_Error_Number = 10;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Rolling_Shutter_Global_Clear(TRUE) failed.");
		return FALSE;
	}
	/* set the direction the sensor reads out in */
	if(!CCD_Command_Set_Sensor_Readout_Mode(Setup_Data.Sensor_Readout_Mode))
	{
		Setup_Error_Number = 11;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Sensor_Readout_Mode(%s) failed.",
			Setup_Data.Sensor_Readout_Mode);
		return FALSE;
	}
	/* configure sensitivity and dynamic range options. */
	if(!CCD_Command_Set_Simple_Pre_Amp_Gain_Control(Setup_Data.Pre_Amp_Gain_Control))
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Simple_Pre_Amp_Gain_Control(%s) failed.",
			Setup_Data.Pre_Amp_Gain_Control);
		return FALSE;
	}

#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Shutdown the connection to the CCD.
 * <ul>
 * <li>We close connection to the CCD camera using CCD_Command_Close.
 * <li>We finalise the libraries used using CCD_Command_Finalise.
 * <ul>
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_command.html#CCD_Command_Finalise
 * @see ccd_command.html#CCD_Command_Close
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Setup_Shutdown(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Shutdown: Started.");
#endif /* LOGGING */
	/* close the open connection to the CCD camera */
	if(!CCD_Command_Close())
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"CCD_Setup_Shutdown: CCD_Command_Close failed.");
		return FALSE;
	}
	/* shutdown the Andor library */
	if(!CCD_Command_Finalise())
	{
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"CCD_Setup_Shutdown: CCD_Command_Finalise failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Shutdown: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Setup_Error_Number
 */
int CCD_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Setup_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Setup:Error(%d) : %s\n",time_string,Setup_Error_Number,Setup_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Setup_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Setup:Error(%d) : %s\n",time_string,
		Setup_Error_Number,Setup_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
