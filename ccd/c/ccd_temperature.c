/* ccd_temperature.c
** Moptop Andor Zyla CCD library
*/
/**
 * Temperature monitoring routines for the Moptop Andor Zyla CCD.
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
#include "log_udp.h"
#include "ccd_general.h"
#include "ccd_command.h"
#include "ccd_temperature.h"

/* hash defines */

/* data types */
/**
 * Internal temperature Data structure. Contains cached temperature data.
 * <dl>
 * <dt>Cached_Temperature</dt> <dd>Cached temperature, in degrees C.</dd>
 * <dt>Cached_Temperature_Date_Stamp</dt> <dd>Date cached data was acquired, of type struct timespec.</dd>
 * <dt>Cached_Temperature_Status_String</dt> <dd>Cached Status string, of length 32 bytes.</dd>
 * <dt>Cached_Temperature_Status_String_Date_Stamp</dt> <dd>Date cached data was acquired, 
 *                                                      of type struct timespec.</dd>
 * </dl>
 * @see #CCD_TEMPERATURE_STATUS
 */
struct Temperature_Struct
{
	double Cached_Temperature;
	struct timespec Cached_Temperature_Date_Stamp;
	char Cached_Temperature_Status_String[32];
	struct timespec Cached_Temperature_Status_String_Date_Stamp;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Internal temperature Data, containing cached temperature data.
 * @see #Temperature_Struct
 */
static struct Temperature_Struct Temperature_Data;
/**
 * Variable holding error code of last operation performed.
 */
static int Temperature_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see ccd_general.html#CCD_GENERAL_ERROR_STRING_LENGTH
 */
static char Temperature_Error_String[CCD_GENERAL_ERROR_STRING_LENGTH] = "";

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Get the current CCD/sensor temperature. We also update the cache in Temperature_Data with a copy of the data.
 * @param sensor_temperature The address of a double. On successful return this contains then sensor temperature
 *        in degrees centigrade.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Temperature_Data
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see ccd_command.html#CCD_Command_Get_Sensor_Temperature
 */
int CCD_Temperature_Get(double *sensor_temperature)
{
	Temperature_Error_Number = 0;
	if(sensor_temperature == NULL)
	{
		Temperature_Error_Number = 1;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get: sensor_temperature was NULL.");
		return FALSE;
	}
	if(!CCD_Command_Get_Sensor_Temperature(sensor_temperature))
	{
		Temperature_Error_Number = 2;
		sprintf(Temperature_Error_String,"CCD_Temperature_Get: CCD_Command_Get_Sensor_Temperature failed.");
		return FALSE;
	}
	/* update cached copy */
	Temperature_Data.Cached_Temperature = (*sensor_temperature);
	clock_gettime(CLOCK_REALTIME,&(Temperature_Data.Cached_Temperature_Date_Stamp));
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Temperature_Get: Temperature = %.3f C.",(*sensor_temperature));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get a string containing the current temperature status of the sensor.
 * This is done by getting the enum index (CCD_Command_Get_Enum_Index on feature "TemperatureStatus"), and then
 * translating this into a string using CCD_Command_Get_Enum_String_By_Index on feature "TemperatureStatus".
 * We also store a copy of the status (and an associated timestamp) in the cache in Temperature_Data.
 * @param status_string A string of length string_length, on success this will contain the temperature status.
 *        According to the Andor_SDK_Manual.pdf, P60, this should be one of:
 *        "Cooler Off", "Cooling", "Stabilised", "Drift", "Not Stabilised", "Fault", "Sensor Over Temperature".
 * @param string_length The length of space allocated for status_string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Temperature_Data
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see ccd_command.html#CCD_Command_Get_Enum_Index
 * @see ccd_command.html#CCD_Command_Get_Enum_String_By_Index
 */
int CCD_Temperature_Get_Temperature_Status_String(char *status_string,int string_length)
{
	int index;

	Temperature_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Temperature_Get_Temperature_Status_String: Started.");
#endif /* LOGGING */
	if(status_string == NULL)
	{
		Temperature_Error_Number = 3;
		sprintf(Temperature_Error_String,
			"CCD_Temperature_Get_Temperature_Status_String: status_string was NULL.");
		return FALSE;
	}
	if(!CCD_Command_Get_Enum_Index("TemperatureStatus",&index))
	{
		Temperature_Error_Number = 4;
		sprintf(Temperature_Error_String,
			"CCD_Temperature_Get_Temperature_Status_String: CCD_Command_Get_Enum_Index failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Temperature_Get_Temperature_Status_String: Temperature Status Enum Index = %d.",
			       index);
#endif /* LOGGING */
	if(!CCD_Command_Get_Enum_String_By_Index("TemperatureStatus",index,status_string,string_length))
	{
		Temperature_Error_Number = 5;
		sprintf(Temperature_Error_String,
			"CCD_Temperature_Get_Temperature_Status_String: CCD_Command_Get_Enum_String_By_Index failed.");
		return FALSE;

	}
	/* update cached copy */
	strncpy(Temperature_Data.Cached_Temperature_Status_String,status_string,32);
	clock_gettime(CLOCK_REALTIME,&(Temperature_Data.Cached_Temperature_Status_String_Date_Stamp));
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Temperature_Get_Temperature_Status_String: Temperature Status = '%s' (Index %d) .",
			       status_string,index);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Test whether the temperature has stabilised. We call CCD_Temperature_Get_Temperature_Status_String,
 * and check whether the returned value is "Stabilised".
 * @return The routine returns TRUE if the status is retrieved and equals "Stabilised", and FALSE if the temperature
 *         status cannot be retrieved or the current status is not "Stabilised". 
 *         You can check whether Temperature_Error_Number is non-zero to determine why this routine retured FALSE.
 * @see #CCD_Temperature_Get_Temperature_Status_String
 */
int CCD_Temperature_Is_Stabilised(void)
{
	char status_string[64];
	
	Temperature_Error_Number = 0;
	if(!CCD_Temperature_Get_Temperature_Status_String(status_string,64))
		return FALSE;
	return (strcmp(status_string,"Stabilised") == 0);
}

/**
 * Return the cached copy of the CCD temperature.
 * @param temperature The address of a double to store the cached CCD temperature.
 * @param cache_date_stamp The address of a struct timespec to store the cached timestamp associated with the
 *        cached temperature (when the CCD temperature was taken).
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Temperature_Data
 */
int CCD_Temperature_Get_Cached_Temperature(double *temperature,struct timespec *cache_date_stamp)
{
	if(temperature != NULL)
		(*temperature) = Temperature_Data.Cached_Temperature;
	if(cache_date_stamp != NULL)
		(*cache_date_stamp) = Temperature_Data.Cached_Temperature_Date_Stamp;
	return TRUE;
}

/**
 * Return the cached copy of the CCD temperature status string.
 * @param temperature_status_string A string pointer to copy the cached temperature status string into. 
 *        This should be at least 32 characters long.
 * @param cache_date_stamp The address of a struct timespec to store the cached timestamp associated with the
 *        cached temperature status string (when the CCD temperature status string was taken).
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Temperature_Data
 */
int CCD_Temperature_Get_Cached_Temperature_Status_String(char *temperature_status_string,
							 struct timespec *cache_date_stamp)
{
	if(temperature_status_string != NULL)
		strcpy(temperature_status_string,Temperature_Data.Cached_Temperature_Status_String);
	if(cache_date_stamp != NULL)
		(*cache_date_stamp) = Temperature_Data.Cached_Temperature_Status_String_Date_Stamp;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Temperature_Error_Number
 */
int CCD_Temperature_Get_Error_Number(void)
{
	return Temperature_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Temperature_Error(void)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s CCD_Temperature:Error(%d) : %s\n",time_string,
		Temperature_Error_Number,Temperature_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see ccd_general.html#CCD_General_Get_Current_Time_String
 */
void CCD_Temperature_Error_String(char *error_string)
{
	char time_string[32];

	CCD_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s CCD_Temperature:Error(%d) : %s\n",time_string,
		Temperature_Error_Number,Temperature_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
