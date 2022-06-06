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
#include <unistd.h>
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
 * <dt>Camera_Setup_Flag</dt> <dd>The camera setup flag to use, when configuring how the shuttering/readout/reset on the
 *                                camera is configured.</dd>
 * <dt>Timestamp_Mode</dt> <dd>What kind of timestamp to include in the read out image data.</dd>
 * <dt>Binning</dt> <dd>The readout binning, stored as an integer. Can be one of 1,2,3,4,8. </dd>
 * <dt>Serial_Number</dt> <dd>An integer containing the serial number retrieved from the camera head
 *                            Retrieved from the camera library during CCD_Setup_Startup.</dd>
 * <dt>Pixel_Width</dt> <dd>A double storing the pixel width in micrometers. Setup from the sensot type 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Pixel_Height</dt> <dd>A double storing the pixel height in micrometers. Setup from the sensot type 
 *                          during CCD_Setup_Startup.</dd>
 * <dt>Sensor_Width</dt> <dd>An integer storing the sensor width in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Sensor_Height</dt> <dd>An integer storing the sensor height in pixels retrieved from the camera during 
 *                       CCD_Setup_Startup.</dd>
 * <dt>Image_Size_Bytes</dt> <dd>An integer storing the image size in bytes.</dd>
 * </dl>
 * @see ccd_command.html#CCD_COMMAND_SETUP_FLAG
 * @see ccd_command.html#CCD_COMMAND_TIMESTAMP_MODE
 */
struct Setup_Struct
{
	int Camera_Board;
	enum CCD_COMMAND_SETUP_FLAG Camera_Setup_Flag;
	enum CCD_COMMAND_TIMESTAMP_MODE Timestamp_Mode;
	int Binning;
	int Serial_Number;
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
 * <dt>Camera_Setup_Flag</dt> <dd>CCD_COMMAND_SETUP_FLAG_GLOBAL_RESET</dd>
 * <dt>Timestamp_Mode</dt> <dd>CCD_COMMAND_TIMESTAMP_MODE_BINARY_ASCII</dd>
 * <dt>Binning</dt> <dd>1</dd>
 * <dt>Serial_Number</dt> <dd>-1</dd>
 * <dt>Pixel_Width</dt> <dd>0.0</dd>
 * <dt>Pixel_Height</dt> <dd>0.0</dd>
 * <dt>Sensor_Width</dt> <dd>0</dd>
 * <dt>Sensor_Height</dt> <dd>0</dd>
 * <dt>Image_Size_Bytes</dt> <dd>0</dd>
 * </dl>
 * @see ccd_command.html#CCD_COMMAND_TIMESTAMP_MODE
 */
static struct Setup_Struct Setup_Data = 
{
	0,CCD_COMMAND_SETUP_FLAG_GLOBAL_RESET,CCD_COMMAND_TIMESTAMP_MODE_BINARY_ASCII,1,-1,0.0,0.0,0,0,0
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
 * Set which camera head (board) to connect to.
 * @param board An integer, the board number of the camera to connect to.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 */
void CCD_Setup_Set_Board(int board)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Board(%d): Started.",board);
#endif /* LOGGING */
	Setup_Data.Camera_Board = board;
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Board: Finished.");
#endif /* LOGGING */
}

/**
 * Set what camera setup flag to use, when configuring how the shuttering/readout/reset on the camera is configured.
 * @param setup_flag A valid CCD_COMMAND_SETUP_FLAG enum, the camera setup flag to use, 
 *        when configuring how the shuttering/readout/reset on the camera is configured.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_COMMAND_SETUP_FLAG
 */
void CCD_Setup_Set_Camera_Setup(enum CCD_COMMAND_SETUP_FLAG setup_flag)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Camera_Setup(0x%x): Started.",setup_flag);
#endif /* LOGGING */
	Setup_Data.Camera_Setup_Flag = setup_flag;
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Camera_Setup: Finished.");
#endif /* LOGGING */
}

/**
 * Set what timestamp data is included in the read-out.
 * @param mode An CCD_COMMAND_TIMESTAMP_MODE enum, the timestamp mode.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_COMMAND_TIMESTAMP_MODE
 */
void CCD_Setup_Set_Timestamp_Mode(enum CCD_COMMAND_TIMESTAMP_MODE mode)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Timestamp_Mode(%d): Started.",mode);
#endif /* LOGGING */
	Setup_Data.Timestamp_Mode = mode;
#if LOGGING > 0
	CCD_General_Log(LOG_VERBOSITY_TERSE,"CCD_Setup_Set_Timestamp_Mode: Finished.");
#endif /* LOGGING */
}

/**
 * Do the initial setup for a PCO camera (specified by the previously set Setup_Data.Camera_Board, 
 * see CCD_Setup_Set_Board).
 * <ul>
 * <li>We initialise the PCO camera library reference object using CCD_Command_Initialise_Camera.
 * <li>We open a connection to the CCD camera using CCD_Command_Open. 
 *     We connect to the camera specified by Setup_Data.Camera_Board.
 * <li>We set the camera shutter readout/reset mode, 
 *     by calling CCD_Command_Set_Camera_Setup with the previously configured Setup_Data.Camera_Setup_Flag as a parameter.
 * <li>We reboot the camera head, to make the camera setup change take effect, by calling CCD_Command_Reboot_Camera.
 * <li>We close the open connection to the camera head by calling CCD_Command_Close.
 * <li>We delete the camera and logger object reference create in CCD_Command_Initialise_Camera by calling CCD_Command_Finalise.
 * <li>We sleep for 10 seconds whilst the camera reboots.
 * <li>We initialise the PCO camera library reference object again using CCD_Command_Initialise_Camera.
 * <li>We open a connection to the CCD camera again using CCD_Command_Open. 
 *     We connect to the camera specified by Setup_Data.Camera_Board.
 * <li>We initialise the PCO camera library grabber reference by calling CCD_Command_Initialise_Grabber.
 * <li>We set the PCO camera to use the current time by calling CCD_Command_Set_Camera_To_Current_Time.
 * <li>We stop any ongoing image acquisitions by calling CCD_Command_Set_Recording_State(FALSE).
 * <li>We reset the camera to a known state by calling CCD_Command_Reset_Settings.
 * <li>We set the camera timestamps using CCD_Command_Set_Timestamp_Mode, 
 *     with the previously configured Setup_Data.Timestamp_Mode as a parameter.
 * <li>We set the camera exposure and delay timebase to microseconds using 
 *     CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US).
 * <li>We set an initial delay and exposure time by calling CCD_Command_Set_Delay_Exposure_Time(0,50);
 * <li>We call CCD_Command_Description_Get_Num_ADCs to get the number of ADCs supported by this camera.
 * <li>If the returned ADC count is greater than one, we call CCD_Command_Set_ADC_Operation(2) to use the extra ADC.
 * <li>We call CCD_Command_Set_Bit_Alignment(0x0001) to set the returned data to be LSB.
 * <li>We call CCD_Command_Set_Noise_Filter_Mode to set noise reduction to off.
 * <li>We call CCD_Command_Description_Get_Max_Horizontal_Size to get Setup_Data.Sensor_Width from the camera
 *     description.
 * <li>We call CCD_Command_Description_Get_Max_Vertical_Size to get Setup_Data.Sensor_Height from the camera
 *     description.
 * <li>We call CCD_Command_Get_Camera_Type to get the camera's serial number from it's head.
 * <li>We call CCD_Command_Description_Get_Sensor_Type to get the camera's sensor type number from it's description.
 * <li>We call CCD_Command_Arm_Camera to update the cameras internal state to take account of the new settings.
 * <li>We call CCD_Command_Grabber_Post_Arm to update the grabber's state to match the camera's state.
 * <ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see #CCD_Setup_Set_Board
 * @see ccd_command.html#CCD_Command_Initialise_Camera
 * @see ccd_command.html#CCD_Command_Open
 * @see ccd_command.html#CCD_Command_Set_Camera_Setup
 * @see ccd_command.html#CCD_Command_Reboot_Camera
 * @see ccd_command.html#CCD_Command_Close
 * @see ccd_command.html#CCD_Command_Finalise
 * @see ccd_command.html#CCD_Command_Initialise_Grabber
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
 * @see ccd_command.html#CCD_Command_Get_Camera_Type
 * @see ccd_command.html#CCD_Command_Description_Get_Sensor_Type
 * @see ccd_command.html#CCD_Command_Arm_Camera
 * @see ccd_command.html#CCD_Command_Grabber_Post_Arm
 * @see ccd_general.html#CCD_General_Log_Format
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Setup_Startup(void)
{
	int adc_count,camera_type,sensor_type,sensor_subtype;

	Setup_Error_Number = 0;
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Started.");
#endif /* LOGGING */
	/* initialise the PCO camera library reference - first time */
	if(!CCD_Command_Initialise_Camera())
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise_Camera failed.");
		return FALSE;
	}
	/* open a connection to the CCD camera - first time */
	if(!CCD_Command_Open(Setup_Data.Camera_Board))
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Open(%d) failed.",Setup_Data.Camera_Board);
		return FALSE;
	}
	/* set the camera shutter readout/reset mode */
	if(!CCD_Command_Set_Camera_Setup(Setup_Data.Camera_Setup_Flag))
	{
		Setup_Error_Number = 27;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Camera_Setup(0x%x) failed.",
			Setup_Data.Camera_Setup_Flag);
		return FALSE;
	}
	/* reboot the camera head, to make the camera setup change take effect */
	if(!CCD_Command_Reboot_Camera())
	{
		Setup_Error_Number = 28;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Reboot_Camera failed.");
		return FALSE;
	}	
	/* close camera connection after reboot */
	if(!CCD_Command_Close())
	{
		Setup_Error_Number = 30;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Close failed.");
		return FALSE;
	}
	/* delete camera and logger object reference before recreating */
	if(!CCD_Command_Finalise())
	{
		Setup_Error_Number = 32;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Finalise failed.");
		return FALSE;
	}
	/* wait 10 seconds before attempting to re-connect. See MA_PCOSDK_V127.pdf,  Section 2.4.8, PCO_SetCameraSetup, P52 */
#if LOGGING > 0
	CCD_General_Log_Format(LOG_VERBOSITY_TERSE,"CCD_Setup_Startup: Sleeping for 10 seconds whilst the camera reboots.");
#endif /* LOGGING */
	sleep(10);
	/* initialise the PCO camera library reference - second time */
	if(!CCD_Command_Initialise_Camera())
	{
		Setup_Error_Number = 33;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise_Camera failed.");
		return FALSE;
	}
	/* open a connection to the CCD camera - second time */
	if(!CCD_Command_Open(Setup_Data.Camera_Board))
	{
		Setup_Error_Number = 34;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Open(%d) failed.",Setup_Data.Camera_Board);
		return FALSE;
	}
	/* initialise the PCO camera library grabber reference */
	if(!CCD_Command_Initialise_Grabber())
	{
		Setup_Error_Number = 26;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Initialise_Grabber failed.");
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
	/* set what timestamp data to include in the read out image */
	if(!CCD_Command_Set_Timestamp_Mode(Setup_Data.Timestamp_Mode))
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"CCD_Setup_Startup: CCD_Command_Set_Timestamp_Mode(mode=%d) failed.",
			Setup_Data.Timestamp_Mode);
		return FALSE;
	}
	/* set exposure and delay timebase to microseconds */
	if(!CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,
		       "CCD_Setup_Startup: CCD_Command_Set_Timebase(CCD_COMMAND_TIMEBASE_US,CCD_COMMAND_TIMEBASE_US) failed.");
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
	/* get and store camera serial number */
	if(!CCD_Command_Get_Camera_Type(&camera_type,&(Setup_Data.Serial_Number)))
	{
		Setup_Error_Number = 22;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: Failed to get camera type / serial number.");
		return FALSE;
	}
	/* get the camera sensor type and subtype */
	if(!CCD_Command_Description_Get_Sensor_Type(&sensor_type,&sensor_subtype))
	{
		Setup_Error_Number = 24;
		sprintf(Setup_Error_String,
			"CCD_Setup_Startup: Failed to get camera sensor type / subtype numbers.");
		return FALSE;
	}
	/* based on sensor type, figure out pixel sizes - PCO library cannot do this directly */
	switch(sensor_type)
	{
		case 0x2002: /* sCMOS CIS1042_V1_FI_BW, as present in our pco.edge 4.2 */
			/* according to the PCO Edge manual MA_PCOEDGE_V225.pdf, P27 */
			Setup_Data.Pixel_Width = 6.5;
			Setup_Data.Pixel_Height = 6.5;
			break;
		default:
			Setup_Error_Number = 25;
			sprintf(Setup_Error_String,"CCD_Setup_Startup: Unknown sensor type 0x%x : "
				"unable to set pixel size.",sensor_type);
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
 * <li>We call CCD_Command_Set_ROI to set the region of interest to match the binning,
 *     with the end positions computed from Setup_Data.Sensor_Width / Setup_Data.Sensor_Height.
 * <li>We call CCD_Command_Arm_Camera to update the camera's internal settings to use the new binning.
 * <li>We call CCD_Command_Grabber_Post_Arm to update the grabber's internal settings to use the new binning.
 * <li>We call CCD_Command_Get_Image_Size_Bytes to update the Setup_Data.Image_Size_Bytes data.
 * </ul>
 * @param bin The binning to apply to the readout. 
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #CCD_SETUP_BINNING_IS_VALID
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see ccd_command.html#CCD_Command_Set_Binning
 * @see ccd_command.html#CCD_Command_Set_ROI
 * @see ccd_command.html#CCD_Command_Arm_Camera
 * @see ccd_command.html#CCD_Command_Grabber_Post_Arm
 * @see ccd_command.html#CCD_Command_Get_Image_Size_Bytes
 */
int CCD_Setup_Dimensions(int bin)
{
	int start_x,start_y,end_x,end_y;
	
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
	/* set the actual binning */
	if(!CCD_Command_Set_Binning(bin,bin))
	{
		Setup_Error_Number = 18;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Set_Binning failed.");
		return FALSE;
	}
	/* set the ROI to the binned pixel area to read out */
	start_x = 1;
	start_y = 1;
	end_x = Setup_Data.Sensor_Width/bin;
	end_y = Setup_Data.Sensor_Height/bin;
	if(!CCD_Command_Set_ROI(start_x,start_y,end_x,end_y))
	{
		Setup_Error_Number = 23;
		sprintf(Setup_Error_String,"CCD_Setup_Dimensions: CCD_Command_Set_ROI failed.");
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
 * Routine to retrieve the camera serial number, saved from the camera in CCD_Setup_Startup to Setup_Data.
 * @param serial_number The address of an integer to copy the camera serial number stored
 *        in Setup_Data.Serial_Number into.
 * @return The string returns TRUE on success and FALSE on failure.
 * @see #Setup_Data
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int CCD_Setup_Get_Serial_Number(int *serial_number)
{
	if(serial_number == NULL)
	{
		Setup_Error_Number = 31;
		sprintf(Setup_Error_String,
			"CCD_Setup_Get_Serial_Number: serial_number was NULL.");
		return FALSE;
	}
	(*serial_number) = Setup_Data.Serial_Number;
	return TRUE;
}

/**
 * Return the physical width of a pixel, as previously configured from the camera sensor type
 * and stored in Setup_Data.
 * @return The physical width of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Width(void)
{
	return Setup_Data.Pixel_Width;
}

/**
 * Return the physical height of a pixel, as previously configured from the camera sensor type
 * and stored in Setup_Data.
 * @return The physical height of a pixel in micrometers.
 * @see #Setup_Data
 */
float CCD_Setup_Get_Pixel_Height(void)
{
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
