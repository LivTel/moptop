/* ccd_setup.cpp
** Moptop PCO CCD library
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
 * The longest SensorReadoutMode value is 25 characters long.
 * The longest SimplePreAmpGainControl value is 40 characters long.
 */
#define SETUP_ENUM_VALUE_STRING_LENGTH (64)

/* data types */
/**
 * Data type holding local data to ccd_setup. This consists of the following:
 * <dl>
 * <dt>Camera_Board</dt> <dd>The board parameter passed to Open_Cam, to determine which camera to connect to.</dd>
 * <dt>Binning</dt> <dd>The readout binning, stored as an integer. Can be one of 1,2,3,4,8. </dd>
 * <dt>Serial_Number</dt> <dd>A character string containing the serial number retrieved from the camera head, 
 *                            of length SETUP_ENUM_VALUE_STRING_LENGTH. 
 *                            Retrieved from the camera library during CCD_Setup_Startup.</dd>
 * <dt>Firmware_Version</dt> <dd>A character string containing the firmware version retrieved from the 
 *                             camera head, of length SETUP_ENUM_VALUE_STRING_LENGTH. 
 *                            Retrieved from the camera library during CCD_Setup_Startup.</dd>
 * <dt>Readout_Time</dt> <dd>An integer storing the length of time to readout the camera in milliseconds. 
 *                       This is retrieved from the camera during startup using CCD_Command_Get_Readout_Time, 
 *                       and converted from decimal seconds to milliseconds.</dd>
 * <dt>Bytes_Per_Pixel</dt> <dd>An integer storing the bytes per pixel retrieved from the camera during startup 
 *                          using CCD_Command_Get_Bytes_Per_Pixel, and converted to an integer.</dd>
 * <dt>Pixel_Width</dt> <dd>A double storing the pixel width in micrometers retrieved from the camera 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Pixel_Height</dt> <dd>A double storing the pixel height in micrometers retrieved from the camera 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Sensor_Width</dt> <dd>An integer storing the sensor width in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Sensor_Height</dt> <dd>An integer storing the sensor height in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Image_Size_Bytes</dt> <dd>An integer storing the image size in bytes.</dd>
 * </dl>
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 */
struct Setup_Struct
{
	int Camera_Board;
	int Binning;
	char Serial_Number[SETUP_ENUM_VALUE_STRING_LENGTH];
	char Firmware_Version[SETUP_ENUM_VALUE_STRING_LENGTH];
	int Readout_Time;
	int Bytes_Per_Pixel;
	double Pixel_Width;
	double Pixel_Height;
	int Sensor_Width;
	int Sensor_Height;
	int Image_Size_Bytes;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Setup_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Camera_Board</dt> <dd>0</dd>
 * <dt>Binning</dt> <dd>1</dd>
 * <dt>Serial_Number</dt> <dd>""</dd>
 * <dt>Firmware_Version</dt> <dd>""</dd>
 * <dt>Readout_Time</dt> <dd>-1</dd>
 * <dt>Bytes_Per_Pixel</dt> <dd>-1</dd>
 * <dt>Pixel_Width</dt> <dd>0.0</dd>
 * <dt>Pixel_Height</dt> <dd>0.0</dd>
 * <dt>Sensor_Width</dt> <dd>0</dd>
 * <dt>Sensor_Height</dt> <dd>0</dd>
 * <dt>Image_Size_Bytes</dt> <dd>0</dd>
 * </dl>
 */
static struct Setup_Struct Setup_Data = 
{
	0,1,"","",-1,-1,0.0,0.0,0,0,0
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
 * Do the initial setup for a PCO camera.
 * <ul>
 * <li>We initialise the libraries used using CCD_Command_Initialise.
 * <li>We open a connection to the CCD camera using CCD_Command_Open.
 * <li>We set the PCO camera to use the current time by calling CCD_Command_Set_Camera_To_Current_Time.
 * <li>We stop any ongoing image acquisitions by calling CCD_Command_Set_Recording_State(FALSE).
 * <li>We reset the camera to a known state by calling CCD_Command_Reset_Settings.
 * <li>We set the camera timestamps to be binary and ASCII using CCD_Command_Set_Timestamp_Mode(2).
 * <li>We set the camera exposure and delay timebase to milliseconds using CCD_Command_Set_Timebase(2,2).
 * <li>We set an initial delay and exposure time by calling CCD_Command_Set_Delay_Exposure_Time(0,50);
 * <li>We call CCD_Command_Description_Get_Num_ADCs to get the number of ADCs supported by this camera.
 * <li>If the returned ADC count is greater than one, we call CCD_Command_Set_ADC_Operation(2) to use the extra ADC.
 * <li>We call CCD_Command_Set_Bit_Alignment(0x0001) to set the returned data to be LSB.
 * <li>We call CCD_Command_Set_Noise_Filter_Mode to set noise reduction to off.
 * <li>We call CCD_Command_Description_Get_Max_Horizontal_Size to get Setup_Data.Sensor_Width from the camera
 *     description.
 * <li>We call CCD_Command_Description_Get_Max_Vertical_Size to get Setup_Data.Sensor_Height from the camera
 *     description.
 * <li>We call CCD_Command_Arm_Camera to update the cameras internal state to take account of the new settings.
 * <li>We call CCD_Command_Grabber_Post_Arm to update the grabber's state to match the camera's state.
 * <ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Initialise
 * @see ccd_command.html#CCD_Command_Open
 * @see ccd_command.html#CCD_Command_Set_Camera_To_Current_Time
 * @see ccd_command.html#CCD_Command_Set_Recording_State
 * @see ccd_command.html#CCD_Command_Reset_Settings
 * @see ccd_command.html#CCD_Command_Set_Timestamp_Mode
 * @see ccd_command.html#CCD_Command_Set_Timebase
 * @see ccd_command.html#CCD_Command_Set_Delay_Exposure_Time
 * @see ccd_command.html#CCD_Command_Description_Get_Num_ADCs
 * @see ccd_command.html#CCD_Command_Set_ADC_Operation
 * @see ccd_command.html#CCD_Command_Set_Bit_Alignment
 * @see ccd_command.html#CCD_Command_Set_Noise_Filter_Mode
 * @see ccd_command.html#CCD_Command_Description_Get_Max_Horizontal_Size
 * @see ccd_command.html#CCD_Command_Description_Get_Max_Vertical_Size
 * @see ccd_command.html#CCD_Command_Arm_Camera
 * @see ccd_command.html#CCD_Command_Grabber_Post_Arm
 * @see ccd_general.html#CCD_General_Log_Format
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Setup_Startup(void)
{
	int adc_count;

	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Started.");
#endif /* LOGGING */
	/* initialise the PCO libraries */
	if(!CCD_Command_Initialise())
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise failed.");
		return FALSE;
	}
	/* open a connection to the CCD camera */
	if(!CCD_Command_Open(Setup_Data.Camera_Board))
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Open(%d) failed.",Setup_Data.Camera_Board);
		return FALSE;
	}
	/* initial configuration of the camera */
	/* set camera to use current time */
	if(!CCD_Command_Set_Camera_To_Current_Time())
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Camera_To_Current_Time failed.");
		return FALSE;
	}
	/* stop any ongoing exposures */
	if(!CCD_Command_Set_Recording_State(FALSE))
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Recording_State(FALSE) failed.");
		return FALSE;
	}
	/* reset camera to a known state */
	if(!CCD_Command_Reset_Settings())
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Reset_Settings failed.");
		return FALSE;
	}
	/* set timestamps to be binary and ASCII */
	if(!CCD_Command_Set_Timestamp_Mode(2))
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Timestamp_Mode(2) failed.");
		return FALSE;
	}
	/* set exposure and delay timebase to milliseconds */
	if(!CCD_Command_Set_Timebase(2,2))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Timebase(2,2) failed.");
		return FALSE;
	}
	/* set an initial delay and exposure time */
	if(!CCD_Command_Set_Delay_Exposure_Time(0,50))
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Delay_Exposure_Time(0,50) failed.");
		return FALSE;
	}
	/* get the number of adc's supported by the camera */
	if(!CCD_Command_Description_Get_Num_ADCs(&adc_count))
	{
		Setup_Error_Number = 10;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Description_Get_Num_ADCs failed.");
		return FALSE;
	}
	if(adc_count > 1)
	{
		if(!CCD_Command_Set_ADC_Operation(2))
		{
			Setup_Error_Number = 11;
			sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_ADC_Operation(2) failed.");
			return FALSE;
		}
	}
	if(!CCD_Command_Set_Bit_Alignment(0x0001)) /* 0x001 = LSB */
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Bit_Alignment(BIT_ALIGNMENT_LSB) failed.");
		return FALSE;
	}
	if(!CCD_Command_Set_Noise_Filter_Mode(0x0000)) /* 0x0000 = off */
	{
		Setup_Error_Number = 13;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: CCD_Command_Set_Noise_Filter_Mode(0x000) failed.");
		return FALSE;
	}
	/* get and store some data from the description, for use later */
	if(!CCD_Command_Description_Get_Max_Horizontal_Size(&Setup_Data.Sensor_Width))
	{
		Setup_Error_Number = 15;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: Failed to get the maximum horizontal size from the description.");
		return FALSE;
	}
	if(!CCD_Command_Description_Get_Max_Vertical_Size(&Setup_Data.Sensor_Height))
	{
		Setup_Error_Number = 21;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: Failed to get the maximum vertical size from the description.");
		return FALSE;
	}	
	/* prepare camera for taking data */
	if(!CCD_Command_Arm_Camera())
	{
		Setup_Error_Number = 14;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Arm_Camera failed.");
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
 * <li>We stop any running acquisition with CCD_Command_Acquisition_Stop.
 * <li>We set the camera triggering mode to software using 
 *     CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE).
 * <li>We close connection to the CCD camera using CCD_Command_Close.
 * <li>We finalise the libraries used using CCD_Command_Finalise.
 * <ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see ccd_command.html#CCD_Command_Acquisition_Stop
 * @see ccd_command.html#CCD_Command_Set_Trigger_Mode
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
		Setup_Error_Number = 17;
		sprintf(Setup_Error_String,"CCD_Setup_Shutdown: CCD_Command_Close failed.");
		return FALSE;
	}
	/* shutdown the PCO library */
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
 * Setup binning and other per exposure configuration.
 * <ul>
 * <li>We use CCD_SETUP_BINNING_IS_VALID to check the binning parameter is a supported binning.
 * <li>We store the binning in Setup_Data.Binning.
 * <li>We call CCD_Command_Set_Binning to set the binning.
 * <li>We call CCD_Command_Arm_Camera to update the camera's internal settings to use the new binning.
 * <li>We call CCD_Command_Grabber_Post_Arm to update the grabber's internal settings to use the new binning.
 * <li>
 * </ul>
 * @param bin The binning to apply to the readout. 
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_SETUP_BINNING_IS_VALID
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Set_Binning
 * @see ccd_command.html#CCD_Command_Arm_Camera
 * @see ccd_command.html#CCD_Command_Grabber_Post_Arm
 */
int CCD_Setup_Dimensions(int bin)
{
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Dimensions: Started.");
#endif /* LOGGING */
	if(!CCD_SETUP_BINNING_IS_VALID(bin))
	{
		Setup_Error_Number = 16;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: Binning %d not valid.",bin);
		return FALSE;
	}
	/* save the binning for later retrieval */
	Setup_Data.Binning = bin;
	/* we may need to retrieve the ROI, and reset it after setting the binning */
	/* set the actual binning */
	if(!CCD_Command_Set_Binning(bin,bin))
	{
		Setup_Error_Number = 18;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Set_Binning failed.");
		return FALSE;
	}
	/* get camera to update it's internal settings */
	if(!CCD_Command_Arm_Camera())
	{
		Setup_Error_Number = 19;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Arm_Camera failed.");
		return FALSE;
	}
	/* get grabber to update to the new binning */
	if(!CCD_Command_Grabber_Post_Arm())
	{
		Setup_Error_Number = 20;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Grabber_Post_Arm failed.");
		return FALSE;
	}
	/* get the new camera image size in bytes */
	/* update the image size in bytes, that changes with binning */
	if(!CCD_Command_Get_Image_Size_Bytes(&(Setup_Data.Image_Size_Bytes)))
	{
		Setup_Error_Number = 29;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Get_Image_Size_Bytes failed.");
		return FALSE;
	}
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Dimensions: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Return the current used readout binning.
 * @return The readout binning, as an integer, Can be one of: 1,2,3,4,8.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Binning(void)
{
	return Setup_Data.Binning;
}

/**
 * Routine to retrieve the camera serial number string, saved from the camera in CCD_Setup_Startup to Setup_Data.
 * @param serial_number_string A string of length string_length bytes to copy the camera serial number string stored
 *        in Setup_Data.Serial_Number into. Ideally should be of length SETUP_ENUM_VALUE_STRING_LENGTH.
 * @param string_length The maximum length of characters that can be stored in serial_number_string.
 * @return The string returns TRUE on success and FALSE on failure (the serial_number_string is not long enough to store
 *         the serial number string).
 * @see #Setup_Data
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Get_Serial_Number(char *serial_number_string,int string_length)
{
	/* diddly */
	if(strlen(Setup_Data.Serial_Number) >= string_length)
	{
		Setup_Error_Number = 31;
		sprintf(Setup_Error_String,
			"CCD_Setup_Get_Serial_Number: serial_number_string length too short (%lu vs %d).",
			strlen(Setup_Data.Serial_Number),string_length);
		return FALSE;
	}
	strcpy(serial_number_string,Setup_Data.Serial_Number);
	return TRUE;
}

/**
 * Routine to retrieve the camera firmware version string, saved from the camera in CCD_Setup_Startup to Setup_Data.
 * @param firmware_version_string A string of length string_length bytes to copy the camera firmware version string 
 *        stored in Setup_Data.Serial_Number into. Ideally should be of length SETUP_ENUM_VALUE_STRING_LENGTH.
 * @param string_length The maximum length of characters that can be stored in firmware_version_string.
 * @return The string returns TRUE on success and FALSE on failure 
 *         (the firmware_version_string is not long enough to store the firmware version string).
 * @see #Setup_Data
 * @see #SETUP_ENUM_VALUE_STRING_LENGTH
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Get_Firmware_Version(char *firmware_version_string,int string_length)
{
	/* diddly */
	if(strlen(Setup_Data.Firmware_Version) >= string_length)
	{
		Setup_Error_Number = 32;
		sprintf(Setup_Error_String,
			"CCD_Setup_Get_Serial_Number: firmware_version_string length too short (%lu vs %d).",
			strlen(Setup_Data.Firmware_Version),string_length);
		return FALSE;
	}
	strcpy(firmware_version_string,Setup_Data.Firmware_Version);
	return TRUE;
}

/**
 * Return the readout time of the CCD, in milliseconds, as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The readout time of the CCD, in milliseconds.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Readout_Time(void)
{
	/* diddly */
	return Setup_Data.Readout_Time;
}

/**
 * Return the bytes per pixel returned during a readout, as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The number of bytes per pixel.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Bytes_Per_Pixel(void)
{
	/* diddly */
	return Setup_Data.Bytes_Per_Pixel;
}

/**
 * Return the physical width of a pixel, as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The physical width of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Width(void)
{
	/* diddly */
	return Setup_Data.Pixel_Width;
}

/**
 * Return the physical height of a pixel, as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The physical height of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Height(void)
{
	/* diddly */
	return Setup_Data.Pixel_Height;
}

/**
 * Return the number of pixels across the sensor (width), as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The sensor width in pixels.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Sensor_Width(void)
{
	return Setup_Data.Sensor_Width;
}

/**
 * Return the number of pixels across the sensor (height), as previously retrieved from the camera head
 * and stored in Setup_Data.
 * @return The sensor height in pixels.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Sensor_Height(void)
{
	return Setup_Data.Sensor_Height;
}

/**
 * Return the size of images returned by the camera head, 
 * as previously retrieved from the camera head and stored in Setup_Data.
 * @return The image size in bytes.
 * @see #Setup_Data
 */
int CCD_Setup_Get_Image_Size_Bytes(void)
{
	return Setup_Data.Image_Size_Bytes;
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
