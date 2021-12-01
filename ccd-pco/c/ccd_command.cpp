/* ccd_command.cpp
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
 * <dt>Description</dt> <dd>The camera description returned from PCO_GetCameraDescriptor.</dd>
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
	SC2_Camera_Description_Response Description;
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
 * <dt>Description</dt> <dd>{}</dd>
 * </dl>
 * @see #Command_Struct
 */
static struct Command_Struct Command_Data = 
{
	NULL,NULL,NULL,0,10000,{}
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
 * <ul>
 * <li>We check the Camera CPco_com_usb instance has been created.
 * <li>We set Command_Data.Camera_Board to the board parameter.
 * <li>We call the Camera's Open_Cam method with the board parameter to open a connection to the board.
 * <li>We construct an instance of CPco_grab_usb attached to the opened camera and assign it to Command_Data.Grabber.
 * <li>We set the Grabber's log instance to Command_Data.PCO_Logger.
 * <li>We set the Grabber's timeout to Command_Data.Grabber_Timeout.
 * <li>We get the camera's description by calling PCO_GetCameraDescriptor and store it in Command_Data.Description.
 * </ul>
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
		sprintf(Command_Error_String,
			"CCD_Command_Open:Camera Open_Cam(board=%d) failed with PCO error code 0x%x.",
			Command_Data.Camera_Board,pco_err);
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
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Open: Getting camera description.");
#endif /* LOGGING */
	pco_err = Command_Data.Camera->PCO_GetCameraDescriptor(&(Command_Data.Description));
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 25;
		sprintf(Command_Error_String,
			"CCD_Command_Open:Camera PCO_GetCameraDescriptor failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
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
 * Prepare the camera to start taking data. All previous ettings are validated and the internal settings of the camera
 * updated.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Arm_Camera(void)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Arm_Camera: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 35;
		sprintf(Command_Error_String,"CCD_Command_Arm_Camera:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_ArmCamera();
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 36;
		sprintf(Command_Error_String,"CCD_Command_Arm_Camera:"
			"Camera PCO_ArmCamera failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Arm_Camera: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Prepare the camera to start taking data. All previous ettings are validated and the internal settings of the camera
 * updated.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Grabber_Post_Arm(void)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Grabber_Post_Arm: Started.");
#endif /* LOGGING */
	if(Command_Data.Grabber == NULL)
	{
		Command_Error_Number = 37;
		sprintf(Command_Error_String,
			"CCD_Command_Grabber_Post_Arm:Grabber CPco_grab_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Grabber->PostArm();
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 38;
		sprintf(Command_Error_String,"CCD_Command_Grabber_Post_Arm:"
			"Grabber PostArm failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Grabber_Post_Arm: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the camera's time to the current time.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Command_Set_Camera_To_Current_Time(void)
{
	DWORD pco_err;

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
	pco_err = Command_Data.Camera->PCO_SetCameraToCurrentTime();
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 13;
		sprintf(Command_Error_String,"CCD_Command_Set_Camera_To_Current_Time:"
			"Camera PCO_SetCameraToCurrentTime failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Command_Set_Camera_To_Current_Time: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the camera's recording state to either TRUE (1) or FALSE (0). This allows the camera to start
 * collecting data (exposures).
 * @param rec_state An integer/boolean, set to TRUE (1) to start recording data and FALSE (0) to stop recording data.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Recording_State(int rec_state)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Set_Recording_State(%d): Started.",rec_state);
#endif /* LOGGING */
	if(!CCD_GENERAL_IS_BOOLEAN(rec_state))
	{
		Command_Error_Number = 10;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Recording_State:Illegal value for rec_state parameter (%d).",rec_state);
		return FALSE;
	}
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 11;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Recording_State:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetRecordingState(rec_state);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 12;
		sprintf(Command_Error_String,"CCD_Command_Set_Recording_State:"
			"Camera PCO_SetRecordingState(%d) failed with PCO error code 0x%x.",
			rec_state,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"CCD_Command_Set_Camera_To_Current_Time: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Reset the cameras settings to a known (default) state.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 */
int CCD_Command_Reset_Settings(void)
{
	DWORD pco_err;

#if LOGGING > 9
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Reset_Settings: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 14;
		sprintf(Command_Error_String,"CCD_Command_Reset_Settings:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_ResetSettingsToDefault();
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 15;
		sprintf(Command_Error_String,"CCD_Command_Reset_Settings:"
			"Camera PCO_ResetSettingsToDefault failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Reset_Settings: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set how the camera records timestamps. 
 * @param mode An integer: 0x0 (off), 0x1 (binary), 0x2 (binary+ASCII), 0x3 (ASCII).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Timestamp_Mode(int mode)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Timestamp_Mode(%d): Started.",mode);
#endif /* LOGGING */
	if((mode < 0)||(mode > 3))
	{
		Command_Error_Number = 16;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Timestamp_Mode:Illegal value for mode parameter (%d).",mode);
		return FALSE;
	}
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 17;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Timestamp_Mode:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetTimestampMode(mode);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 18;
		sprintf(Command_Error_String,"CCD_Command_Set_Timestamp_Mode:"
			"Camera PCO_SetTimestampMode(%d) failed with PCO error code 0x%x.",
			mode,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Timestamp_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the units used for delays and exposures.
 * @param delay_timebase An integer, used to set the units used for delays: 0x0 (ns), 0x1 (us), 0x2 (ms).
 * @param exposure_timebase An integer, used to set the units used for exposures: 0x0 (ns), 0x1 (us), 0x2 (ms).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Timebase(int delay_timebase,int exposure_timebase)
{
	WORD exp_timebase,del_timebase;
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Timebase(delay=%d,exposure=%d): Started.",
			       delay_timebase,exposure_timebase);
#endif /* LOGGING */
	if((delay_timebase < 0)||(delay_timebase > 2))
	{
		Command_Error_Number = 19;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Timebase:Illegal value for delay_timebase parameter (%d).",delay_timebase);
		return FALSE;
	}
	if((exposure_timebase < 0)||(exposure_timebase > 2))
	{
		Command_Error_Number = 20;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Timebase:Illegal value for exposure_timebase parameter (%d).",
			exposure_timebase);
		return FALSE;
	}
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 21;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Timebase:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	del_timebase = delay_timebase;
	exp_timebase = exposure_timebase;
	pco_err = Command_Data.Camera->PCO_SetTimebase(del_timebase,exp_timebase);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 22;
		sprintf(Command_Error_String,"CCD_Command_Set_Timebase:"
			"Camera PCO_SetTimebase(%d,%d) failed with PCO error code 0x%x.",
			del_timebase,exp_timebase,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Timebase: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the delay and exposure time.
 * @param delay_time An integer, used to set the delay, in units previously specified by CCD_Command_Set_Timebase.
 * @param exposure_time An integer, used to set the exposure length, 
 *        in units previously specified by CCD_Command_Set_Timebase.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #CCD_Command_Set_Timebase
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Delay_Exposure_Time(int delay_time,int exposure_time)
{
	DWORD exp_time_dw,delay_time_dw;
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Set_Delay_Exposure_Time(delay=%d,exposure=%d): Started.",
			       delay_time,exposure_time);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 23;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Delay_Exposure_Time:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	delay_time_dw = delay_time;
	exp_time_dw = exposure_time;
	pco_err = Command_Data.Camera->PCO_SetDelayExposure(delay_time_dw,exp_time_dw);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 24;
		sprintf(Command_Error_String,"CCD_Command_Set_Delay_Exposure_Time:"
			"Camera PCO_SetDelayExposure(%d,%d) failed with PCO error code 0x%x.",
			delay_time_dw,exp_time_dw,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Set_Delay_Exposure_Time: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the number of analogue to digital converters used.
 * @param num_adcs An integer: either 0x1 or 0x2, the number of ADCs to use. 2 is faster, 1 is more linear.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_ADC_Operation(int num_adcs)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_ADC_Operation(%d): Started.",num_adcs);
#endif /* LOGGING */
	if((num_adcs < 1)||(num_adcs > 2))
	{
		Command_Error_Number = 26;
		sprintf(Command_Error_String,
			"CCD_Command_Set_ADC_Operation:Illegal value for num_adcs parameter (%d).",num_adcs);
		return FALSE;
	}
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 27;
		sprintf(Command_Error_String,
			"CCD_Command_Set_ADC_Operation:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetADCOperation(num_adcs);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 28;
		sprintf(Command_Error_String,"CCD_Command_Set_ADC_Operation:"
			"Camera PCO_SetADCOperation(%d) failed with PCO error code 0x%x.",
			num_adcs,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_ADC_Operation: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the bit alignment used for the output image data.
 * @param bit_alignment An integer: 0x0 (MSB) or 0x1 (LSB).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Bit_Alignment(int bit_alignment)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Bit_Alignment(%d): Started.",bit_alignment);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 31;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Bit_Alignment:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetBitAlignment(bit_alignment);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 32;
		sprintf(Command_Error_String,"CCD_Command_Set_Bit_Alignment:"
			"Camera PCO_SetBitAlignment(%d) failed with PCO error code 0x%x.",
			bit_alignment,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Bit_Alignment: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set what image corrects the camera performs.
 * @param mode An integer: 0x0000 (off), 0x0001 (noise filter on), 0x0101 (noise filter on + hot pixel correction).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Noise_Filter_Mode(int mode)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Noise_Filter_Mode(%d): Started.",mode);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 33;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Noise_Filter_Mode:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetNoiseFilterMode(mode);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 34;
		sprintf(Command_Error_String,"CCD_Command_Set_Noise_Filter_Mode:"
			"Camera PCO_SetNoiseFilterMode(%d) failed with PCO error code 0x%x.",mode,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Noise_Filter_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set how camera exposures are triggered
 * @param mode An enum of type CCD_COMMAND_TRIGGER_MODE. Used to select external or internal trigger modes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #CCD_COMMAND_TRIGGER_MODE
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE mode)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Trigger_Mode(%d): Started.",mode);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 39;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Trigger_Mode:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetTriggerMode(mode);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 40;
		sprintf(Command_Error_String,"CCD_Command_Set_Trigger_Mode:"
			"Camera PCO_SetTriggerMode(%d) failed with PCO error code 0x%x.",mode,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Trigger_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the camera binning.
 * @param bin_x An integer, the horizontal binning.
 * @param bin_y An integer, the vertical binning.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Set_Binning(int bin_x,int bin_y)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Binning(%d,%d): Started.",bin_x,bin_y);
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 41;
		sprintf(Command_Error_String,
			"CCD_Command_Set_Binning:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_SetBinning(bin_x,bin_y);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 42;
		sprintf(Command_Error_String,"CCD_Command_Set_Binning:"
			"Camera PCO_SetBinning(%d,%d) failed with PCO error code 0x%x.",bin_x,bin_y,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Set_Trigger_Mode: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Call the Grabber to acquire 1 frame from the camera, and place the data into the passed in image buffer.
 * @param image_buffer The address of some allocated memory to hold the read out image.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Grabber_Acquire_Image_Async_Wait(void *image_buffer)
{
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Grabber_Acquire_Image_Async_Wait: Started.");
#endif /* LOGGING */
	if(Command_Data.Grabber == NULL)
	{
		Command_Error_Number = 58;
		sprintf(Command_Error_String,
			"CCD_Command_Grabber_Acquire_Image_Async_Wait:Grabber CPco_grab_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Grabber->Acquire_Image_Async_wait(image_buffer);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 59;
		sprintf(Command_Error_String,"CCD_Command_Grabber_Acquire_Image_Async_Wait:"
			"Grabber Acquire_Image_Async_wait(%p) failed with PCO error code 0x%x.",image_buffer,pco_err);
		return FALSE;
	}
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Grabber_Acquire_Image_Async_Wait: Finished.");
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
 * Get the number of analogue to digital convertors inside the camera, as returned from it's description
 * (retrieved from the camera head when opening a connection to the camera, and stored in Command_Data.Description).
 * @param adc_count The address of an integer to store the numbers of analogue to digital convertors inside the camera.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Description_Get_Num_ADCs(int *adc_count)
{
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Description_Get_Num_ADCs: Started.");
#endif /* LOGGING */
	if(adc_count == NULL)
	{
		Command_Error_Number = 29;
		sprintf(Command_Error_String,
			"CCD_Command_Description_Get_Num_ADCs:adc_count was NULL.");
		return FALSE;
	}
	/* check camera instance has been created, if so open should have been called,
	** and the Description field retrieved from the camera head. */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 30;
		sprintf(Command_Error_String,
			"CCD_Command_Description_Get_Num_ADCs:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	(*adc_count) = Command_Data.Description.wNumADCsDESC;
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Description_Get_Num_ADCs returned %d ADCs.",
			       (*adc_count));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the minimum exposure length of the camera, as returned from it's description
 * (retrieved from the camera head when opening a connection to the camera, and stored in Command_Data.Description).
 * @param minimum_exposure_length_s The address of an double to store the minimum exposure length in seconds.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_NS
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Exposure_Time_Min(double *minimum_exposure_length_s)
{
	DWORD min_expose_dw_ns;
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Exposure_Time_Min: Started.");
#endif /* LOGGING */
	if(minimum_exposure_length_s == NULL)
	{
		Command_Error_Number = 54;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Exposure_Time_Min:minimum_exposure_length_s was NULL.");
		return FALSE;
	}
	/* check camera instance has been created, if so open should have been called,
	** and the Description field retrieved from the camera head. */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 55;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Exposure_Time_Min:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	min_expose_dw_ns = Command_Data.Description.dwMinExposureDESC;
	/* The minimum exposure length in the description structure is in nanoseconds, convert to seconds */
	(*minimum_exposure_length_s) = ((double)min_expose_dw_ns)/((double)CCD_GENERAL_ONE_SECOND_NS);
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Exposure_Time_Min returned %.2f s minimum exposure length.",
			       (*minimum_exposure_length_s));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the maximum exposure length of the camera, as returned from it's description
 * (retrieved from the camera head when opening a connection to the camera, and stored in Command_Data.Description).
 * @param maximum_exposure_length_s The address of an double to store the maximum exposure length in seconds.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_GENERAL_ONE_SECOND_MS
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Exposure_Time_Max(double *maximum_exposure_length_s)
{
	DWORD max_expose_dw_ms;
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Exposure_Time_Max: Started.");
#endif /* LOGGING */
	if(maximum_exposure_length_s == NULL)
	{
		Command_Error_Number = 56;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Exposure_Time_Max:maximum_exposure_length_s was NULL.");
		return FALSE;
	}
	/* check camera instance has been created, if so open should have been called,
	** and the Description field retrieved from the camera head. */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 57;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Exposure_Time_Max:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	max_expose_dw_ms = Command_Data.Description.dwMaxExposureDESC;
	/* The maximum exposure length in the description structure is in milliseconds, convert to seconds */
	(*maximum_exposure_length_s) = ((double)max_expose_dw_ms)/((double)CCD_GENERAL_ONE_SECOND_MS);
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Exposure_Time_Max returned %.2f s maximum exposure length.",
			       (*maximum_exposure_length_s));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the actual size of the image that the camera will return, given the current binning settings.
 * @param image_width The address of an integer to store the width of the image, in pixels.
 * @param image_height The address of an integer to store height of the image, in pixels.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Actual_Size(int *image_width,int *image_height)
{
	DWORD pco_err;
	DWORD image_width_w,image_height_w;
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Actual_Size: Started.");
#endif /* LOGGING */
	if(image_width == NULL)
	{
		Command_Error_Number = 43;
		sprintf(Command_Error_String,"CCD_Command_Get_Actual_Size:image_width was NULL.");
		return FALSE;
	}
	if(image_height == NULL)
	{
		Command_Error_Number = 44;
		sprintf(Command_Error_String,"CCD_Command_Get_Actual_Size:image_height was NULL.");
		return FALSE;
	}
	/* check camera instance has been created, if so open should have been called,
	** and the Description field retrieved from the camera head. */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 45;
		sprintf(Command_Error_String,"CCD_Command_Get_Actual_Size:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_GetActualSize(&image_width_w,&image_height_w);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 46;
		sprintf(Command_Error_String,"CCD_Command_Get_Temperature:PCO_GetActualSize failed(0x%x).",pco_err);
		return FALSE;
	}
	(*image_width) = image_width_w;
	(*image_height) = image_height_w;
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Actual_Size returned width = %d pixels, height = %d pixels.",
			       (*image_width),(*image_height));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the size of the image in bytes.
 * @param image_size The address opf an integer to return the image size, in bytes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see ccd_general.html#CCD_General_Log
 * @see ccd_general.html#CCD_General_Log_Format
 */ 
int CCD_Command_Get_Image_Size_Bytes(int *image_size)
{
	int image_width,image_height;
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Image_Size_Bytes: Started.");
#endif /* LOGGING */
	if(image_size == NULL)
	{
		Command_Error_Number = 47;
		sprintf(Command_Error_String,"CCD_Command_Get_Image_Size_Bytes:image_size was NULL.");
		return FALSE;
	}
	/* get the binned image dimensions from the camera */
	if(!CCD_Command_Get_Actual_Size(&image_width,&image_height))
		return FALSE;
	(*image_size) = image_width*image_height*sizeof(WORD);
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			       "CCD_Command_Get_Image_Size_Bytes: Returned image size in bytes of %d.",(*image_size));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set how camera exposures are triggered
 * @param mode An enum of type CCD_COMMAND_TRIGGER_MODE. Used to select external or internal trigger modes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #CCD_COMMAND_TRIGGER_MODE
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String 
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Trigger_Mode(enum CCD_COMMAND_TRIGGER_MODE *mode)
{
	DWORD pco_err;
	WORD mode_w;
	
#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_VERBOSE,"CCD_Command_Get_Trigger_Mode: Started.");
#endif /* LOGGING */
	if(mode == NULL)
	{
		Command_Error_Number = 48;
		sprintf(Command_Error_String,"CCD_Command_Get_Trigger_Mode:mode was NULL.");
		return FALSE;
	}
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 49;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Trigger_Mode:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_GetTriggerMode(&mode_w);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 50;
		sprintf(Command_Error_String,"CCD_Command_Get_Trigger_Mode:"
			"Camera PCO_GetTriggerMode failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
	switch(mode_w)
	{
		case 0x0: /* software/  auto */
			(*mode) = CCD_COMMAND_TRIGGER_MODE_INTERNAL;
			break;
		case 0x2: /* external exposure start & software trigger */
			(*mode) = CCD_COMMAND_TRIGGER_MODE_EXTERNAL;
			break;
		default:
			Command_Error_Number = 51;
			sprintf(Command_Error_String,"CCD_Command_Get_Trigger_Mode:"
				"Camera PCO_GetTriggerMode returned unsupported trigger mode 0x%x.",mode_w);
			
			break;
	}
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_VERBOSE,
			       "CCD_Command_Get_Trigger_Mode: Finished and returned trigger mode %d.",(*mode));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the currently set delay and exposure time.
 * @param delay_time The address of an integer, to be filled in with the current delay time, 
 *                    in units previously specified by CCD_Command_Set_Timebase.
 * @param exposure_time The address of an integer, to be filled in with the current exposure length, 
 *        in units previously specified by CCD_Command_Set_Timebase.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Data
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #CCD_Command_Set_Timebase
 * @see ccd_general.html#CCD_General_Log_Format
 */
int CCD_Command_Get_Delay_Exposure_Time(int *delay_time,int *exposure_time)
{
	DWORD exp_time_dw,delay_time_dw;
	DWORD pco_err;

#if LOGGING > 5
	CCD_General_Log(LOG_VERBOSITY_INTERMEDIATE,"CCD_Command_Get_Delay_Exposure_Time: Started.");
#endif /* LOGGING */
	if(Command_Data.Camera == NULL)
	{
		Command_Error_Number = 52;
		sprintf(Command_Error_String,
			"CCD_Command_Get_Delay_Exposure_Time:Camera CPco_com_usb instance not created.");
		return FALSE;
	}
	pco_err = Command_Data.Camera->PCO_GetDelayExposure(&delay_time_dw,&exp_time_dw);
	if(pco_err != PCO_NOERROR)
	{
		Command_Error_Number = 53;
		sprintf(Command_Error_String,"CCD_Command_Get_Delay_Exposure_Time:"
			"Camera PCO_GetDelayExposure failed with PCO error code 0x%x.",pco_err);
		return FALSE;
	}
	if(delay_time != NULL)
		(*delay_time) = delay_time_dw;
	if(exposure_time != NULL)
		(*exposure_time) = exp_time_dw;
#if LOGGING > 5
	CCD_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
			    "CCD_Command_Get_Delay_Exposure_Time: Finished returning delay time %d, exposure time %d.",
			       delay_time_dw,exp_time_dw);
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
