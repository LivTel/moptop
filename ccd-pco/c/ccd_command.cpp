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
#include "VersionNo.h"
#include "Cpco_com.h"
#include "Cpco_grab_usb.h"
#include "ccd_general.h"
#include "ccd_command.h"

/* data types */
/**
 * Data type holding local data to ccd_command. This consists of the following:
 * <dl>
 * <dt>Camera</dt> <dd>The instance of CPco_com used to communicate with the PCO camera. </dd>
 * <dt>Grabber</dt> <dd>The instance of CPco_grab_usb used to grab images from the PCO camera. </dd>
 * <dt>PCO_Logger</dt> <dd>The instance of CPco_Log used to receive logging from the PCO library.</dd>
 * <dt>Camera_Board</dt> <dd>The board number passed to Open_Cam.</dd>
 * <dt>Grabber_Timeout</dt> <dd>The timeout for grabbing images, in milliseconds.</dd>
 * </dl>
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 */
struct Command_Struct
{
	CPco_com *Camera;
	CPco_grab_usb* Grabber;
	CPco_Log* PCO_Logger;
	int Camera_Board;
	int Grabber_Timeout;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Command_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Camera</dt> <dd>NULL</dd>
 * <dt>Grabber</dt> <dd>NULL</dd>
 * <dt>PCO_Logger</dt> <dd>NULL</dd>
 * <dt>Camera_Board</dt> <dd>0</dd>
 * <dt>Grabber_Timeout</dt> <dd>10000</dd>
 * </dl>
 * @see #Command_Struct
 */
static struct Command_Struct Command_Data = 
{
	NULL,NULL,NULL,0,10000
};

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
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Initialise(void)
{
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Started.");
#endif /* LOGGING */
	Command_Error_Number = 0;
#if LOGGING > 1
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Initialise: Creating CPco_com_usb instance.");
#endif /* LOGGING */
	Command_Data.Camera = new CPco_com_usb();
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"CCD_Command_Initialise:Creating CPco_com_usb instance failed.");
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Initialise: Creating CPco_Log instance.");
#endif /* LOGGING */
	Command_Data.PCO_Logger = new CPco_Log("pco_camera_grab.log");
	if(Command_Data.PCO_Logger == NULL)
	{
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"CCD_Command_Initialise:Creating CPco_Log instance failed.");
		return FALSE;
	}
#if LOGGING > 1
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Initialise: Initialising CPco_Log instance.");
#endif /* LOGGING */
	Command_Data.PCO_Logger->set_logbits(0x3);
	Command_Data.Camera->SetLog(Command_Data.PCO_Logger);
#if LOGGING > 1
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Initialise: Creating CPco_Log instance.");
#endif /* LOGGING */
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Command_Initialise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Finalise (finish using) the CCD library. 
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Finalise(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Started.");
#endif /* LOGGING */
	if(Command_Data.Grabber != NULL)
		delete Command_Data.Grabber;
	if(Command_Data.Camera != NULL)
		delete Command_Data.Camera;
	if(Command_Data.PCO_Logger != NULL)
		delete Command_Data.PCO_Logger;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Finalise: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Open a connection to the PCO camera and get a camera handle.
 * @param board Which camera to connect to.
 * @return The routine returns TRUE on success and FALSE if it fails.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_GENERAL_ONE_MILLISECOND_NS
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Open(int board)
{
	DWORD pco_err;
	
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Open: Started for board %d.",board);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"CCD_Command_Open:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	Command_Data.Camera_Board = board;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Open: Calling Open_Cam(%d).",
			       Command_Data.Camera_Board);
#endif /* LOGGING */
	pco_err = Command_Data.Camera->Open_Cam(Command_Data.Camera_Board);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"CCD_Command_Open:Camera Open_Cam(board=%d) failed.",
			Command_Data.Camera_Board);
		return FALSE;
	}
	/* create grabber for opened camera */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Open: Creating Grabber for camera.");
#endif /* LOGGING */
	Command_Data.Grabber = new CPco_grab_usb((CPco_com_usb*)(Command_Data.Camera));
	if(Command_Data.Grabber == NULL)
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,"CCD_Command_Open:Creating CPco_grab_usb instance failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Open: Configuring Grabber.");
#endif /* LOGGING */
	Command_Data.Grabber->SetLog(Command_Data.PCO_Logger);
	Command_Data.Grabber->Set_Grabber_Timeout(Command_Data.Grabber_Timeout);
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
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Close(void)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 5;
		sprintf(Command_Error_String,"CCD_Command_Close:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	Command_Data.Camera->Close_Cam();
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Command_Close: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the camera's time to the current time.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Command_Set_Camera_To_Current_Time(void)
{
#if LOGGING > 9
	CCD_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Command_Set_Camera_To_Current_Time: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 7;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Camera_To_Current_Time:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	Command_Data.Camera->PCO_SetCameraToCurrentTime();
#if LOGGING > 9
	CCD_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Command_Set_Camera_To_Current_Time: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the camera/sensor/psu temperatures from the camera.
 * @param valid_sensor_temp The address of an integer, on a successful return from this function this will contain
 *        TRUE if a valid sensor temperature was read, and FALSE if it was not read. This address can be NULL
 *        if the sensor temperature is not required.
 * @param sensor_temp The address of a double, on a successful return from this function this will contain
 *        the sensor temperature in degrees Centigrade if it was read successfully. This address can be NULL
 *        if the sensor temperature is not required.
 * @param camera_temp The address of an integer, on a successful return from this function this will contain
 *        the camera temperature in degrees Centigrade. This address can be NULL
 *        if the camera temperature is not required.
 * @param valid_psu_temp The address of an integer, on a successful return from this function this will contain
 *        TRUE if a valid PSU temperature was read, and FALSE if it was not read. This address can be NULL
 *        if the PSU temperature is not required.
 * @param psu_temp The address of an integer, on a successful return from this function this will contain
 *        the PSU temperature in degrees Centigrade. This address can be NULL
 *        if the PSU temperature is not required.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Command_Get_Temperature(int *valid_sensor_temp,double *sensor_temp,int *camera_temp,
				int *valid_psu_temp,int *psu_temp)
{
	DWORD pco_err;
	SHORT ccd_temp_s,cam_temp_s,psu_temp_s;
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Temperature: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Temperature:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_GetTemperature(&ccd_temp_s,&cam_temp_s,&psu_temp_s);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 9;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Temperature:PCO_GetTemperature failed(0x%x).",pco_err);
		return FALSE;
	}
	/* PCO_GetTemperature returns:
	** A short for the CCD temperature in tenths of degree centigrade, 
	**          which is (SHORT)(-32768) when reading the temperature fails.
	** A short for the camera temperature in degrees centigrade.
	** A short for the power supply temperature in degrees centigrade, which id 0 when the sensor does not exist.
	*/
	if((valid_sensor_temp != NULL)&&(sensor_temp != NULL))
	{
		if(ccd_temp_s != (SHORT)(-32768))
		{
			(*valid_sensor_temp) = TRUE;
			(*sensor_temp) = ((double)ccd_temp_s)/10.0;
		}
		else
		{
			(*valid_sensor_temp) = FALSE;
			(*sensor_temp) = 0.0;
		}
	}
	if(camera_temp != NULL)
	{
		(*camera_temp) = (int)cam_temp_s;
	}
	if((valid_psu_temp != NULL)&&(psu_temp != NULL))
	{
		if(psu_temp_s != (SHORT)(0x000))
		{
			(*valid_psu_temp) = TRUE;
			(*psu_temp) = (int)psu_temp_s;
		}
		else
		{
			(*valid_psu_temp) = FALSE;
			(*psu_temp) = 0;
		}
	}
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Temperature: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the size of the image in bytes.
 * @param image_size The address opf an integer to return the image size, in bytes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 */ 
int CCD_Command_Get_Image_Size_Bytes(int *image_size)
{
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
