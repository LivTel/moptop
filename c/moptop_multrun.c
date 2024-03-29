/* moptop_multrun.c
** Moptop multrun routines
*/
/**
 * Multrun routines for the moptop program.
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
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "ccd_buffer.h"
#include "ccd_command.h"
#include "ccd_exposure.h"
#include "ccd_fits_filename.h"
#include "ccd_fits_header.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

#include "filter_wheel_command.h"
#include "filter_wheel_config.h"

#include "pirot_command.h"
#include "pirot_setup.h"

#include "moptop_config.h"
#include "moptop_fits_header.h"
#include "moptop_general.h"
#include "moptop_multrun.h"

/* hash defines */
/**
 * Length of FITS filename string.
 */
#define MULTRUN_FITS_FILENAME_LENGTH  (256)
/**
 * Length of cached filter name and id data.
 */
#define MULTRUN_FILTER_NAME_LENGTH    (32)
/**
 * Length of cached rotator speed.
 */
#define MULTRUN_ROTATOR_SPEED_LENGTH  (32)

/* data types */
/**
 * Data type holding local data to moptop multruns.
 * <dl>
 * <dt>Rotator_Speed</dt> <dd>A copy of the rotator speed string used to configure the rotator, 
 *                            used for filling in the FITS headers. Of length MULTRUN_ROTATOR_SPEED_LENGTH.</dd>
 * <dt>Rotator_Run_Velocity</dt> <dd>A copy of the rotator run velocity used to configure the rotator.</dd>
 * <dt>Rotator_Step_Angle</dt> <dd>A copy of the rotator step angle used to configure the rotator.</dd>
 * <dt>Filter_Position</dt> <dd>A copy of the filter position taken at the start of the multrun, 
 *                              used for filling in FITS headers.</dd>
 * <dt>Filter_Name</dt> <dd>A copy of the filter name taken at the start of the multrun, used for filling in FITS headers. 
 *                          Of length MULTRUN_FILTER_NAME_LENGTH.</dd>
 * <dt>Filter_Id</dt> <dd>A copy of the filter Id taken at the start of the multrun, used for filling in FITS headers. 
 *                        Of length MULTRUN_FILTER_NAME_LENGTH.</dd>
 * <dt>CCD_Temperature</dt> <dd>A copy of the current CCD temperature, taken at the start of a multrun. 
 *                              Used to populate FITS headers.</dd>
 * <dt>CCD_Temperature_Status_String</dt> <dd>A copy of the current CCD temperature status, 
 *                                            taken at the start of a multrun. Used to populate FITS headers.</dd>
 * <dt>Requested_Exposure_Length</dt> <dd>A copy of the per-frame requested exposure length (in seconds) 
 *                                        used to configure the CCD camera. Used to populate FITS headers.</dd>
 * <dt>Image_Index</dt> <dd>Which frame in the multrun we are currently working on.</dd>
 * <dt>Image_Count</dt> <dd>The number of FITS images we are expecting to generate in the current multrun.</dd>
 * <dt>Multrun_Start_Time</dt> <dd>A timestamp taken the first time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate). Used for calculating TELAPSE.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>A timestamp taken the last time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate).</dd>
 * <dt>Rotation_Number</dt> <dd>A number representing which rotation of the rotator we are on 
 *                              (from 1 to Multrun_Data.Image_Count/images_per_cycle).</dd>
 * <dt>Sequence_Number</dt> <dd>A number representing which image in the current rotation we are on 
 *                              (from 1 to images_per_cycle).</dd>
 * <dt>Flip_X</dt> <dd>A boolean, if TRUE flip the image data in the X (horizontal) direction.</dd>
 * <dt>Flip_Y</dt> <dd>A boolean, if TRUE flip the image data in the Y (vertical) direction.</dd>
 * </dl>
 * @see #MULTRUN_ROTATOR_SPEED_LENGTH
 * @see #MULTRUN_FILTER_NAME_LENGTH
 */
struct Multrun_Struct
{
	char Rotator_Speed[MULTRUN_ROTATOR_SPEED_LENGTH];
	double Rotator_Run_Velocity;
	double Rotator_Step_Angle;
	int Filter_Position;
	char Filter_Name[MULTRUN_FILTER_NAME_LENGTH];
	char Filter_Id[MULTRUN_FILTER_NAME_LENGTH];
	double CCD_Temperature;
	char CCD_Temperature_Status_String[64];
	double Requested_Exposure_Length;
	int Image_Index;
	int Image_Count;
	struct timespec Multrun_Start_Time;
	struct timespec Exposure_Start_Time;
	int Rotation_Number;
	int Sequence_Number;
	int Flip_X;
	int Flip_Y;
};
	
/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Multrun Data holding local data to moptop multruns.
 * <dl>
 * <dt>Rotator_Speed</dt>                 <dd>""</dd>
 * <dt>Rotator_Run_Velocity</dt>          <dd>0.0</dd>
 * <dt>Rotator_Step_Angle</dt>            <dd>0.0</dd>
 * <dt>Filter_Position</dt>               <dd>-1</dd>
 * <dt>Filter_Name</dt>                   <dd>""</dd>
 * <dt>Filter_Id</dt>                     <dd>""</dd>
 * <dt>CCD_Temperature</dt>               <dd>0.0</dd>
 * <dt>CCD_Temperature_Status_String</dt> <dd>""</dd>
 * <dt>Requested_Exposure_Length</dt>     <dd>0.0</dd>
 * <dt>Image_Index</dt>                   <dd>0</dd>
 * <dt>Image_Count</dt>                   <dd>0</dd>
 * <dt>Multrun_Start_Time</dt>            <dd>{0,0}</dd>
 * <dt>Exposure_Start_Time</dt>           <dd>{0,0}</dd>
 * <dt>Rotation_Number</dt>               <dd>0</dd>
 * <dt>Sequence_Number</dt>               <dd>0</dd>
 * <dt>Flip_X</dt>                        <dd>FALSE</dd>
 * <dt>Flip_Y</dt>                        <dd>FALSE</dd>
 * </dl>
 * @see #Multrun_Struct
 */
static struct Multrun_Struct Multrun_Data =
{
	"",0.0,0.0,-1,"","",0.0,"",0.0,0,0,{0,0},{0,0},0,0,FALSE,FALSE
};

/**
 * Is a multrun in progress.
 */
static int Multrun_In_Progress = FALSE;
/**
 * Abort any multrun currently in progress.
 */
static int Moptop_Abort = FALSE;

/* internal functions */
static int Multrun_Acquire_Images(int do_standard,char ***filename_list,int *filename_count);
static int Multrun_Get_Fits_Filename(int images_per_cycle,int do_standard,char *filename,int filename_length);
static int Multrun_Write_Fits_Image(int do_standard,double pco_exposure_length_s,
				    struct timespec exposure_end_time,int camera_image_number,
				    struct timespec camera_timestamp,
				    double requested_rotator_angle,double rotator_start_angle,
				    double rotator_end_angle,double rotator_difference,
				    unsigned char *image_buffer,
				    int image_buffer_length,char *filename);
/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to set the exposure length to be used for the Multrun. This is the exposure length _per-frame_, 
 * i.e. it depends on the rotorspeed, and leads to allow enough time to readout and save each image 
 * before the rotator triggers the start of the next exposure.
 * <ul>
 * <li>The camera exposure length is set using CCD_Exposure_Length_Set.
 * <li>The requested exposure length is stored in Multrun_Data.Requested_Exposure_Length 
 *     (for later use in FITS headers).
 * </ul>
 * @param exposure_length_s The exposure length to use for each frame, in seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_command.html#CCD_Exposure_Length_Set
 */
int Moptop_Multrun_Exposure_Length_Set(double exposure_length_s)
{
	/* configure the CCD camera exposure length 
	** Note this might be modified by the PCO library. */
	if(!CCD_Exposure_Length_Set(exposure_length_s))
	{
		Moptop_General_Error_Number = 640;
		sprintf(Moptop_General_Error_String,
			"Moptop_Multrun_Exposure_Length_Set: Failed to set exposure length %.3f s.",exposure_length_s);
		return FALSE;
	}
	/* Save the requested exposure length for later inclusion in the FITS headers */
	Multrun_Data.Requested_Exposure_Length = exposure_length_s;
	return TRUE;
}

/**
 * Routine to set the cached multrun filter name data, which is used to fill in the FILTER1 keyword when saving
 * multrun FITS images. This routine should only be used by the C layer _not_ attached to the filter wheel,
 * the C layer that has access to the filter wheel queries it's position and caches the filter name as part
 * of the "multrun_setup" command.
 * @param filter_name A string representation of the filter name to be cached.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_Multrun_Filter_Name_Set(char *filter_name)
{
	if(filter_name == NULL)
	{
		Moptop_General_Error_Number = 642;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Filter_Name_Set: filter_name was NULL.");
		return FALSE;
	}
	if(strlen(filter_name) >= MULTRUN_FILTER_NAME_LENGTH)
	{
		Moptop_General_Error_Number = 643;
		sprintf(Moptop_General_Error_String,
			"Moptop_Multrun_Filter_Name_Set: filter_name '%s' was too long (%ld, vs %d).",
			filter_name,strlen(filter_name),MULTRUN_FILTER_NAME_LENGTH);
		return FALSE;
	}
	strcpy(Multrun_Data.Filter_Name,filter_name);
	return TRUE;
}

/**
 * Routine to configure how the image is orientated on readout.
 * @param flip_x A boolean, if TRUE the readout is flipped in the X (horizontal) direction after readout.
 * @param flip_y A boolean, if TRUE the readout is flipped in the Y (vertical) direction after readout.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_Multrun_Flip_Set(int flip_x,int flip_y)
{
	if(!MOPTOP_GENERAL_IS_BOOLEAN(flip_x))
	{
		Moptop_General_Error_Number = 650;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Flip_Set: flip_x (%d) not a boolean.",flip_x);
		return FALSE;
	}
	if(!MOPTOP_GENERAL_IS_BOOLEAN(flip_y))
	{
		Moptop_General_Error_Number = 651;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Flip_Set: flip_y (%d) not a boolean.",flip_y);
		return FALSE;
	}
	Multrun_Data.Flip_X = flip_x;
	Multrun_Data.Flip_Y = flip_y;
	return TRUE;
}

/**
 * Routine to setup the Multrun.
 * <ul>
 * <li>Increment the FITS filename multrun number and return it. We do this before checking whether the 
 *     rotator is in the right start position, so the multrun numbers do not become mismatched across C layers.
 * <li>Increment the FITS filename run number.
 * <li>If the rotator is enabled, we configure it from values previously cached in the config rotorspeed command,
 *     by calling PIROT_Setup_Rotator.
 * <li>If the filter wheel is enabled, we get the current filter wheel position using Filter_Wheel_Command_Get_Position.
 * <li>If the filter wheel is enabled, We get the current filter name using Filter_Wheel_Config_Position_To_Name.
 * <li>We get the filter id associated with the filter name (either previously cached or just retrieved) by calling
 *     Filter_Wheel_Config_Name_To_Id.
 * <li>We get and cache the current CCD temperature using CCD_Temperature_Get to store the temperature in 
 *     Multrun_Data.CCD_Temperature.
 * <li>We get and cache the current CCD temperature status string using CCD_Temperature_Get_Temperature_Status_String 
 *     to store the temperature in Multrun_Data.CCD_Temperature_Status_String.
 * <li>We set the PCO camera to use the current time by calling CCD_Command_Set_Camera_To_Current_Time.
 *     We must do this every multrun as the internal camera clocks drift with respect to real time, see Fault 2745.
 * <li>We configure whether to flip the output image data before writing to disk. We use Moptop_Config_Get_Boolean
 *     to retrieve the 'moptop.multrun.image.flip.x' and 'moptop.multrun.image.flip.y' config from the config file,
 *     and then call Moptop_Multrun_Flip_Set to set the flip flags for later use in the readout code.
 * </ul>
 * @param multrun_number The address of an integer to store the multrun number we expect to use for this multrun.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see #Moptop_Multrun_Flip_Set
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_config.html#Moptop_Config_Filter_Wheel_Is_Enabled
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Camera_To_Current_Time
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Multrun
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Run
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Rotator
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Position_To_Name
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Name_To_Id
 */
int Moptop_Multrun_Setup(int *multrun_number)
{
	int flip_x,flip_y;
	
	if(multrun_number == NULL)
	{
		Moptop_General_Error_Number = 604;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: multrun_number was NULL.");
		return FALSE;
	}
	/* increment the multrun number */
	CCD_Fits_Filename_Next_Multrun();
	(*multrun_number) = CCD_Fits_Filename_Multrun_Get();
	/* increment the run number (effectively which rotation we are on) to one */
	CCD_Fits_Filename_Next_Run();
	/* if the rotator is enabled, set it up and move it to the right start position 
	** This picks up the velocity and step angle configured in the config rotorspeed command
	** (PIROT_Setup_Rotator_Run_Velocity/PIROT_Setup_Trigger_Step_Angle). */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		if(!PIROT_Setup_Rotator())
		{
			Moptop_General_Error_Number = 605;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: PIROT_Setup_Rotator failed.");
			return FALSE;
		}
	}
	/* get and save the current filter wheel settings, if enabled */
	if(Moptop_Config_Filter_Wheel_Is_Enabled())
	{
		if(!Filter_Wheel_Command_Get_Position(&(Multrun_Data.Filter_Position)))
		{
			Moptop_General_Error_Number = 626;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: Failed to get filter wheel position.");
			return FALSE;		
		}
		if(!Filter_Wheel_Config_Position_To_Name(Multrun_Data.Filter_Position,Multrun_Data.Filter_Name))
		{
			Moptop_General_Error_Number = 627;
			sprintf(Moptop_General_Error_String,
				"Moptop_Multrun_Setup: Failed to get filter wheel name from it's position.");
			return FALSE;		
		}
	}
	/* else do nothing, the config filter command should have updated Multrun_Data.Filter_Name */
	/* Convert the filter name Multrun_Data.Filter_Name into an Id Multrun_Data.Filter_Id.
	** We do the conversion here rather than Moptop_Multrun_Filter_Name_Set, as
	** Moptop_Multrun_Filter_Name_Set only gets called on C layers that have no filter wheel (moptop2/moptop4).
	** We have also modified Moptop_Config_Load to _always_ call Filter_Wheel_Config_Initialise so we can
	** use the name -> id mapping here. */
	if(!Filter_Wheel_Config_Name_To_Id(Multrun_Data.Filter_Name,Multrun_Data.Filter_Id))
	{
		Moptop_General_Error_Number = 647;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup:"
			"Failed to find filter name id for filter name '%s'.",Multrun_Data.Filter_Name);
		return FALSE;
	}
	/* get current CCD temperature/status and store it for later */
	if(!CCD_Temperature_Get(&(Multrun_Data.CCD_Temperature)))
	{
		Moptop_General_Error_Number = 628;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: Failed to get CCD temperature.");
		return FALSE;		
	}
	if(!CCD_Temperature_Get_Temperature_Status_String(Multrun_Data.CCD_Temperature_Status_String,64))
	{
		Moptop_General_Error_Number = 629;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: Failed to get CCD temperature status string.");
		return FALSE;		
	}
	/* set the camera's internal clock to the current system time.
	** We do this for every multrun as the camera's internal clock drifts with respect to real time, see
	** Fault #2745 for details */
	if(!CCD_Command_Set_Camera_To_Current_Time())
	{
		Moptop_General_Error_Number = 621;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: Failed to set the camera to the current time.");
		return FALSE;
	}
	/* configure flipping of output image */
	if(!Moptop_Config_Get_Boolean("moptop.multrun.image.flip.x",&flip_x))
		return FALSE;		
	if(!Moptop_Config_Get_Boolean("moptop.multrun.image.flip.y",&flip_y))
		return FALSE;		
	Moptop_Multrun_Flip_Set(flip_x,flip_y);
	return TRUE;
}

/**
 * Routine to actually do a multrun for one machine.
 * <ul>
 * <li>We check that one (and only one) of use_exposure_length and use_exposure_count are set to TRUE.
 * <li>We initialise Moptop_Abort to FALSE, and Moptop_In_Progress to TRUE.
 * <li>If use_exposure_length is TRUE, we:
 *     <ul>
 *     <li>Retrieve the configured rotator_run_velocity using Moptop_Multrun_Rotator_Run_Velocity_Get.
 *     <li>Compute the number of rotations (rotation_count) = (exposure_length_ms/1000.0)/(360.0/rotator_run_velocity)
 *     <li>Check the computed rotation_count is at least one.
 *     </ul>
 * <li>If use_exposure_count is set, we set the rotation_count to exposure_count.
 * <li>We check the computed/passed in rotation_count is in the range (1..100).
 * <li>We retrieve the configured rotator_step_angle using Moptop_Multrun_Rotator_Step_Angle_Get.
 * <li>We compute the number of exposures (Multrun_Data.Image_Count) = rotation_count*(360.0/trigger_step_angle).
 * <li>We compute the rotator_end_position = (360.0 * rotation_count)-PIROT_SETUP_ROTATOR_TOLERANCE. 
 *     We stop short on the last exposure to avoid an extra trigger/frame.
 * <li>We set the camera to externally trigger by calling CCD_Command_Set_Trigger_Mode with parameter
 *     CCD_COMMAND_TRIGGER_MODE_EXTERNAL.
 * <li>We update the cameras internal settings with it's previously configured data by calling CCD_Command_Arm_Camera.
 * <li>We update the image grabbers internal data to match by calling CCD_Command_Grabber_Post_Arm.
 * <li>We tell the camera to start responding to triggers by calling CCD_Command_Set_Recording_State(TRUE).
 * <li>If the rotator is enabled (Moptop_Config_Rotator_Is_Enabled):
 *     <ul>
 *     <li>We enable the rotator hardware triggers using PIROT_Command_TRO.
 *     <li>We command the rotator to start moving towards it's end position using 
 *         PIROT_Command_MOV(rotator_end_position).
 *     </ul>
 * <li>We acquire the image date using  Multrun_Acquire_Images.
 * <li>We stop the camera recording image by calling CCD_Command_Set_Recording_State(FALSE).
 * <li>We set the camera back to internal triggers by calling CCD_Command_Set_Trigger_Mode with parameter
 *     CCD_COMMAND_TRIGGER_MODE_INTERNAL.
 * <li>If the rotator is enabled (Moptop_Config_Rotator_Is_Enabled):
 *     <ul>
 *     <li>We disable the rotator hardware triggers using PIROT_Command_TRO.
 *     </ul>
 * <li>We set Moptop_In_Progress to FALSE.
 * </ul>
 * @param exposure_length_ms The length of time to open the shutter for in milliseconds. 
 * @param use_exposure_length A boolean, if TRUE use the exposure length.
 * @param exposure_count The number of exposures to take. In this case, used to determine the number of complete
 *        rotations to use.
 * @param use_exposure_count A boolean, if TRUE use the exposure count.
 * @param do_standard A boolean, if TRUE this is an observation of a standard, otherwise it is not.
 * @param filename_list The address of a list of filenames of FITS images acquired during this multrun.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the exposure succeeds, returns FALSE if an error occurs or the exposure is aborted.
 * @see #Moptop_Data
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see #Multrun_Acquire_Images
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Step_Angle_Get
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Arm_Camera
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Grabber_Post_Arm
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Recording_State
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Trigger_Mode
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_TRO
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_MOV
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_SETUP_ROTATOR_TOLERANCE
 */
int Moptop_Multrun(int exposure_length_ms,int use_exposure_length,int exposure_count,int use_exposure_count,
		   int do_standard,char ***filename_list,int *filename_count)
{
	double rotator_run_velocity,trigger_step_angle,rotator_end_position;
	int retval,rotation_count;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_TERSE,"MULTRUN",
				  "(exposure_length_ms = %d,use_exposure_length = %d,exposure_count = %d,"
				  "use_exposure_count = %d,do_standard = %d,filename_list = %p,filename_count = %p)"
				  " started.",
				  exposure_length_ms,use_exposure_length,exposure_count,use_exposure_count,
				  do_standard,filename_list,filename_count);
#endif
	/* check arguments */
	if(filename_list == NULL)
	{
		Moptop_General_Error_Number = 600;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun: filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)
	{
		Moptop_General_Error_Number = 601;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun: filename_count was NULL.");
		return FALSE;
	}
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* ensure one of use_exposure_length and use_exposure_count is set true, otherwise return an error */
	if((use_exposure_length && use_exposure_count) || ((!use_exposure_length) && (!use_exposure_count)))
	{
		Moptop_General_Error_Number = 602;
		sprintf(Moptop_General_Error_String,
			"Moptop_Multrun:Illegal arguments: use_exposure_length = %d, use_exposure_count = %d.",
			use_exposure_length,use_exposure_count);
		return FALSE;
	}
	/* initialise abort and in progress flags */
	Moptop_Abort = FALSE;
	Multrun_In_Progress = TRUE;
	/* if using exposure lengths, convert this into a number of rotations (exposure_count) */
	if(use_exposure_length)
	{
		/* rotator_run_velocity is in degrees/s.
		** exposure_length_ms is in milliseconds */
		rotator_run_velocity = Moptop_Multrun_Rotator_Run_Velocity_Get();
		rotation_count = (int)((((double)exposure_length_ms)/1000.0)/(360.0/rotator_run_velocity));
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
			  "Using exposure length %d ms, rotator run velocity %.2f deg/s, therefore rotation count %d.",
					  exposure_length_ms,rotator_run_velocity,rotation_count);
#endif
		if(rotation_count < 1)
		{
			Multrun_In_Progress = FALSE;
			Moptop_General_Error_Number = 603;
			sprintf(Moptop_General_Error_String,
				"Moptop_Multrun:Exposure length %d ms, rotator run velocity %.2f deg/s, "
				"gives rotation count less than 1.",exposure_length_ms,rotator_run_velocity);
			return FALSE;
		}
	}
	/* if using exposure counts, number of rotations is the exposure count */
	if(use_exposure_count)
		rotation_count = exposure_count;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
				  "Using rotation count %d.",rotation_count);
#endif
	/* We need to do at least one rotation. The rotator cannot do more than 100 rotations in this configuration. */
	if((rotation_count < 1)||(rotation_count > 100))
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 646;
		sprintf(Moptop_General_Error_String,
			"Moptop_Multrun:Using Exposure length %d ms,Exposure Count %d,"
			"gives rotation count %d out of range (1..100).",
			exposure_length_ms,exposure_count,rotation_count);
		return FALSE;		
	}
	/* how many exposures are we expecting */
	trigger_step_angle = Moptop_Multrun_Rotator_Step_Angle_Get();
	Multrun_Data.Image_Count = rotation_count*(360.0/trigger_step_angle);
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
				  "We are expecting %d frames.",Multrun_Data.Image_Count);
#endif
	/* what is the rotator end position? */
	/* stop short on last exposure to avoid an extra trigger/frame */
	rotator_end_position = (360.0 * rotation_count)-PIROT_SETUP_ROTATOR_TOLERANCE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
				  "Rotator end position %.3f.",rotator_end_position);
#endif
	/* turn on camera external triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_EXTERNAL))
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 644;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to set camera trigger mode to external.");
		return FALSE;
	}
	/* get the camera ready with the new settings */
	if(!CCD_Command_Arm_Camera())
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 652;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:CCD_Command_Arm_Camera failed.");
		return FALSE;
	}
	/* update the grabber so thats ready */
	if(!CCD_Command_Grabber_Post_Arm())
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 609;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:CCD_Command_Grabber_Post_Arm failed.");
		return FALSE;
	}
	/* start taking data */
	if(!CCD_Command_Set_Recording_State(TRUE))
	{
		Multrun_In_Progress = FALSE;
		CCD_Command_Set_Recording_State(FALSE);
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL);
		Moptop_General_Error_Number = 615;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to start camera recording data.");
		return FALSE;
	}
	/* only configure and move the rotator, this this is the C layer with it enabled */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		/* enable rotator hardware trigger */
		if(!PIROT_Command_TRO(TRUE))
		{
			Multrun_In_Progress = FALSE;
			CCD_Command_Set_Recording_State(FALSE);
			CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL);
			Moptop_General_Error_Number = 606;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to enable rotator triggering.");
			return FALSE;
		}
		/* move rotator to it's final position */
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
					  "Moving Rotator to end position %.3f.",rotator_end_position);
#endif
		if(!PIROT_Command_MOV(rotator_end_position))
		{
			Multrun_In_Progress = FALSE;
			CCD_Command_Set_Recording_State(FALSE);
			CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL);
			PIROT_Command_TRO(FALSE);
			Moptop_General_Error_Number = 607;
			sprintf(Moptop_General_Error_String,
				"Moptop_Multrun:Failed to move rotator to end position %.2f.",rotator_end_position);
			return FALSE;
		}
	}/* end if rotator enabled */
	/* acquire camera images */
	retval = Multrun_Acquire_Images(do_standard,filename_list,filename_count);
	if(retval == FALSE)
	{
		CCD_Command_Set_Recording_State(FALSE);
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL);
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Multrun_In_Progress = FALSE;
		return FALSE;
	}
	/* stop recording data */
	if(!CCD_Command_Set_Recording_State(FALSE))
	{
		Multrun_In_Progress = FALSE;
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL);
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Moptop_General_Error_Number = 616;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to stop camera recording.");
		return FALSE;
	}
	/* turn off external camera triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL))
	{
		Multrun_In_Progress = FALSE;
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Moptop_General_Error_Number = 645;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to set camera trigger mode to internal.");
		return FALSE;
	}		
	/* only configure and move the rotator, this this is the C layer with it enabled */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		/* disable rotator hardware trigger */
		if(!PIROT_Command_TRO(FALSE))
		{
			Multrun_In_Progress = FALSE;
			Moptop_General_Error_Number = 608;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to disable rotator triggering.");
			return FALSE;
		}
	}
	Multrun_In_Progress = FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_TERSE,"MULTRUN","finished.");
#endif
	return TRUE;
}

/**
 * Abort a currently running multrun.
 * <ul>
 * <li>Sets Moptop_Abort to TRUE.
 * <li>If Multrun_In_Progress is true:
 *     <ul>
 *     <li>Stop camera acquisition using CCD_Command_Set_Recording_State(FALSE).
 *     <li>If therotator is enabled (Moptop_Config_Rotator_Is_Enabled), stop the rotator triggering using 
 *         PIROT_Command_TRO.
 *     </ul>
 * <li>Returns Multrun_In_Progress (i.e. whether there was a multrun in progress to be aborted).
 * </ul>
 * @return The routine returns TRUE if the multrun was aborted, FALSE otherwise.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Recording_State
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_TRO
 */
int Moptop_Multrun_Abort(void)
{
	Moptop_Abort = TRUE;
	if(Multrun_In_Progress)
	{
		/* stop the camera recording */
		if(!CCD_Command_Set_Recording_State(FALSE))
		{
			Moptop_General_Error_Number = 617;
			sprintf(Moptop_General_Error_String,
				"Moptop_Multrun_Abort:Failed to stop camera recording.");
			return FALSE;
		}
		/* stop rotator triggering, if the rotator is enabled */
		if(Moptop_Config_Rotator_Is_Enabled())
		{
			if(!PIROT_Command_TRO(FALSE))
			{
				Moptop_General_Error_Number = 618;
				sprintf(Moptop_General_Error_String,
					"Moptop_Multrun_Abort:Failed to stop rotator triggering.");
				return FALSE;
			}
			/* diddly stop the rotator moving? */
		}
	}/* end if Multrun_In_Progress */
	/* allow aborted multrun to call CCD_Command_Flush rather than call it here */
	return TRUE;
}

/**
 * Return whether a MULTRUN is in progress.
 * @return TRUE if a multrun is in progress, FALSE otherwise.
 * @see #Multrun_In_Progress
 */
int Moptop_Multrun_In_Progress(void)
{
	return Multrun_In_Progress;
}

/**
 * Return the total number of exposures expected to be generated in the current/last multrun.
 * @return The number of images/frames expected.
 * @see #Multrun_Data
 */
int Moptop_Multrun_Count_Get(void)
{
	return Multrun_Data.Image_Count;
}

/**
 * Return the exposure length of an individual frame in the multrun. This is dependant on the rotator velocity.
 * @return The exposure length of an individual frame in milliseconds.
 * @see #Multrun_Data
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 */
int Moptop_Multrun_Per_Frame_Exposure_Length_Get(void)
{
	return (int)(Multrun_Data.Requested_Exposure_Length*((double)MOPTOP_GENERAL_ONE_SECOND_MS));
}

/**
 * Return the exposure start time timestamp of the last exposure in the multrun.
 * @param exposure_start_time The address of a timespec structure to fill with the start time timestamp.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 */
int Moptop_Multrun_Exposure_Start_Time_Get(struct timespec *exposure_start_time)
{
	if(exposure_start_time == NULL)
	{
		Moptop_General_Error_Number = 641;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Exposure_Start_Time_Get:exposure_start_time was NULL.");
		return FALSE;
	}
	(*exposure_start_time) = Multrun_Data.Exposure_Start_Time;
	return TRUE;
}

/**
 * Return which exposure in the multrun we are on.
 * @return The exposure index in the multrun.
 * @see #Multrun_Data
 */
int Moptop_Multrun_Exposure_Index_Get(void)
{
	return Multrun_Data.Image_Index;
}

/**
 * Return the multrun number (in the generated FITS filenames) of this multrun.
 * @return The current multrun number.
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 */
int Moptop_Multrun_Multrun_Get(void)
{
	return CCD_Fits_Filename_Multrun_Get();
}

/**
 * Return the run number (in the generated FITS filenames) of this multrun. This is the rotation we are on.
 * @return The current run number (rotation number).
 * @see #Multrun_Data
 */
int Moptop_Multrun_Run_Get(void)
{
	return Multrun_Data.Rotation_Number;	
}

/**
 * Return the window number (in the generated FITS filenames) of this multrun. 
 * This is the image within a rotation we are on.
 * @return The current run number (the image within a rotation).
 * @see #Multrun_Data
 */
int Moptop_Multrun_Window_Get(void)
{
	return Multrun_Data.Sequence_Number;	
}

/**
 * Take a copy of the rotator speed string used to configure the speed of the rotator. This is stored in
 * Multrun_Data.Rotator_Speed.
 * @param rotator_speed The rotator speed string used to configure the rotator. This is usually "slow" or "fast".
 * @return The routine returns TRUE on success and FALSE on failure (rotator_speed was NULL, rotator_speed string
 *         was too long).
 * @see #MULTRUN_ROTATOR_SPEED_LENGTH
 * @see #Multrun_Data
 */
int Moptop_Multrun_Rotator_Speed_Set(char *rotator_speed)
{
	if(rotator_speed == NULL)
	{
		Moptop_General_Error_Number = 648;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Rotator_Speed_Set: rotator_speed was NULL.");
		return FALSE;
	}
	if(strlen(rotator_speed) > MULTRUN_ROTATOR_SPEED_LENGTH)
	{
		Moptop_General_Error_Number = 649;
		sprintf(Moptop_General_Error_String,
			"Moptop_Multrun_Rotator_Speed_Set: rotator_speed string was too long (%lu vs %d).",
			strlen(rotator_speed),MULTRUN_ROTATOR_SPEED_LENGTH);
		return FALSE;
	}		
	strcpy(Multrun_Data.Rotator_Speed,rotator_speed);
	return TRUE;
}

/**
 * Set the stored rotator run velocity (used for computations inside Multrun_Multrun).
 * @param velocity The rotator velocity configured, in degrees/s.
 * @see #Multrun_Data
 */
void Moptop_Multrun_Rotator_Run_Velocity_Set(double velocity)
{
	Multrun_Data.Rotator_Run_Velocity = velocity;
}

/**
 * Set the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @param step_angle The rotator step angle configured, in degrees.
 * @see #Multrun_Data
 */
void Moptop_Multrun_Rotator_Step_Angle_Set(double step_angle)
{
	Multrun_Data.Rotator_Step_Angle = step_angle;
}

/**
 * Retrieve the cached rotator speed string used to configure the speed of the rotator. This is stored in
 * Multrun_Data.Rotator_Speed.
 * @param rotator_speed A string, on return to be filled with the cached rotator speed string 
 *                      used to configure the rotator. This is usually "slow" or "fast". 
 *                      The string should have an allocated length of MULTRUN_ROTATOR_SPEED_LENGTH,
 *                      though currently only 5 characters are used.
 * @see #MULTRUN_ROTATOR_SPEED_LENGTH
 * @see #Multrun_Data
 */
void Moptop_Multrun_Rotator_Speed_Get(char *rotator_speed)
{
	strcpy(rotator_speed,Multrun_Data.Rotator_Speed);
}

/**
 * Get the stored rotator run velocity (used for computations inside Multrun_Multrun).
 * @return The stored rotator velocity in degrees/s.
 * @see #Multrun_Data
 */
double Moptop_Multrun_Rotator_Run_Velocity_Get(void)
{
	return Multrun_Data.Rotator_Run_Velocity;
}

/**
 * Get the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @return The stored rotator step angle, in degrees.
 * @see #Multrun_Data
 */
double Moptop_Multrun_Rotator_Step_Angle_Get(void)
{
	return Multrun_Data.Rotator_Step_Angle;
}

/**
 * Flip the image data in the X direction.
 * @param ncols The number of columns on the CCD.
 * @param nrows The number of rows on the CCD.
 * @param exposure_data The image data received from the CCD, as an unsigned short. 
 *        The data in this array is flipped in the X direction.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 */
void Moptop_Multrun_Flip_X(int ncols,int nrows,unsigned short *exposure_data)
{
	int x,y;
	unsigned short int tempval;

#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun_Flip_X",LOG_VERBOSITY_INTERMEDIATE,"MULTRUN",
				  "Started flipping image of size (%d,%d) in X.",ncols,nrows);
#endif
	/* for each row */
	for(y=0;y<nrows;y++)
	{
		/* for the first half of the columns.
		** Note the middle column will be missed, this is OK as it
		** does not need to be flipped if it is in the middle */
		for(x=0;x<(ncols/2);x++)
		{
			/* Copy exposure_data[x,y] to tempval */
			tempval = *(exposure_data+(y*ncols)+x);
			/* Copy exposure_data[ncols-(x+1),y] to exposure_data[x,y] */
			*(exposure_data+(y*ncols)+x) = *(exposure_data+(y*ncols)+(ncols-(x+1)));
			/* Copy tempval = exposure_data[ncols-(x+1),y] */
			*(exposure_data+(y*ncols)+(ncols-(x+1))) = tempval;
		}
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("multrun","moptop_multrun.c","Moptop_Multrun_Flip_X",LOG_VERBOSITY_INTERMEDIATE,"MULTRUN",
			   "Finished.");
#endif
}

/**
 * Flip the image data in the Y direction.
 * @param ncols The number of columns on the CCD.
 * @param nrows The number of rows on the CCD.
 * @param exposure_data The image data received from the CCD, as an unsigned short. 
 *        The data in this array is flipped in the Y direction.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 */
void Moptop_Multrun_Flip_Y(int ncols,int nrows,unsigned short *exposure_data)
{
	int x,y;
	unsigned short int tempval;

#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun_Flip_Y",LOG_VERBOSITY_INTERMEDIATE,"MULTRUN",
				  "Started flipping image of size (%d,%d) in Y.",ncols,nrows);
#endif
	/* for the first half of the rows.
	** Note the middle row will be missed, this is OK as it
	** does not need to be flipped if it is in the middle */
	for(y=0;y<(nrows/2);y++)
	{
		/* for each column */
		for(x=0;x<ncols;x++)
		{
			/* Copy exposure_data[x,y] to tempval */
			tempval = *(exposure_data+(y*ncols)+x);
			/* Copy exposure_data[x,nrows-(y+1)] to exposure_data[x,y] */
			*(exposure_data+(y*ncols)+x) = *(exposure_data+(((nrows-(y+1))*ncols)+x));
			/* Copy tempval = exposure_data[x,nrows-(y+1)] */
			*(exposure_data+(((nrows-(y+1))*ncols)+x)) = tempval;
		}
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("multrun","moptop_multrun.c","Moptop_Multrun_Flip_Y",LOG_VERBOSITY_INTERMEDIATE,"MULTRUN",
			   "Finished.");
#endif
}

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to actually acquire the externally triggered images with the rotator moving.
 * <ul>
 * <li>We check the filename_list and filename_count are not NULL and initialise them.
 * <li>We get the camera exposure length using CCD_Exposure_Length_Get.
 * <li>We calculate a timeout as being four times the length of time between two triggers.
 * <li>We loop over the Multrun_Data.Image_Count, using Multrun_Data.Image_Index as an index counter (for status reporting):
 *     <ul>
 *     <li>We take a timestamp and store it in Multrun_Data.Exposure_Start_Time.
 *     <li>If this is the first exposure in the multrun we set Multrun_Data.Multrun_Start_Time to the same timestamp.
 *     <li>We compute the theoretical rotator start angle (within a rotation) and store it in rotator_start_angle.
 *     <li>We compute which rotation we are on and store it in Multrun_Data.Rotation_Number.
 *     <li>We compute the image we are taking within the current rotation and store it in Multrun_Data.Sequence_Number.
 *     <li>We wait for a readout by calling CCD_Command_Grabber_Acquire_Image_Async_Wait_Timeout with a timeout
 *         four times the time between two triggers.
 *     <li>If the rotator is configured (Moptop_Config_Rotator_Is_Enabled) we retrieve the actual final rotator 
 *         position using PIROT_Command_Query_POS, and use it compute the rotator_difference and the
 *         rotator_end_angle (the curent position in the current rotation).
 *     <li>If the rotator is _not_ configured  we compute a theoretical rotator_difference and rotator_end_angle.
 *     <li>We get the camera image number from the image metadata using CCD_Command_Get_Image_Number_From_Metadata.
 *     <li>We get the camera image timestamp from the image metadata using CCD_Command_Get_Timestamp_From_Metadata.
 *     <li>We get an exposure end timestamp and store it in exposure_end_time.
 *     <li>We call Multrun_Get_Fits_Filename to generate a new FITS filename.
 *     <li>We call Multrun_Write_Fits_Image to write the image data to the generated FITS filename.
 *     <li>We add the generated filename to the filename list using CCD_Fits_Filename_List_Add.
 *     <li>We increment requested_rotator_angle to the theoretical rotator start angle of the next image.
 *     <li>We check whether the multrun has been aborted (Moptop_Abort).
 *     </ul>
 * <li>
 * </ul>
 * @param do_standard A boolean, if TRUE this is an observation of a standard, otherwise it is not.
 * @param filename_list The address of a list of filenames of FITS images acquired during this multrun.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #MULTRUN_FITS_FILENAME_LENGTH
 * @see #Moptop_Abort
 * @see #Moptop_Multrun_Rotator_Step_Angle_Get
 * @see #Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see #Multrun_Get_Fits_Filename
 * @see #Multrun_Write_Fits_Image
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Get_Image_Buffer
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Grabber_Acquire_Image_Async_Wait_Timeout
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Image_Number_From_Metadata
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Timestamp_From_Metadata
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Add
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Image_Size_Bytes
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_Query_POS
 */
static int Multrun_Acquire_Images(int do_standard,char ***filename_list,int *filename_count)
{
	struct timespec exposure_end_time,camera_timestamp;
	char filename[MULTRUN_FITS_FILENAME_LENGTH];
	unsigned int timeout_ms;
	double requested_rotator_angle = 0.0;
	double rotator_start_angle,current_rotator_position,rotator_difference,rotator_end_angle;
	double camera_clock_difference;
	double pco_exposure_length_s;
	int camera_image_number;
	int images_per_cycle;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Acquire_Images",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","started with image count %d, do_standard = %d.",Multrun_Data.Image_Count,do_standard);
#endif
	/* check arguments */
	if(filename_list == NULL)
	{
		Moptop_General_Error_Number = 619;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images: filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)
	{
		Moptop_General_Error_Number = 620;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images: filename_count was NULL.");
		return FALSE;
	}
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* get exposure length used by the pco camera */
	if(!CCD_Exposure_Length_Get(&pco_exposure_length_s))
	{
		Moptop_General_Error_Number = 610;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to get PCO exposure length.");
		return FALSE;
	}
	/* time taken between two triggers is rotator_step_angle/rotator_run_velocity 
	** Lets make it four times that in milliseconds, 
	** at two times the wait on moptop2 can timeout for the first frame
	** as it starts earlier than that on moptop1 (as moptop1 is also starting the rotator etc...) */
	timeout_ms = (int)(4.0*(Moptop_Multrun_Rotator_Step_Angle_Get()/Moptop_Multrun_Rotator_Run_Velocity_Get()))*
		MOPTOP_GENERAL_ONE_SECOND_MS;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Acquire_Images",LOG_VERBOSITY_VERBOSE,
				  "MULTRUN","Using acquire timeout of %d ms.",timeout_ms);
#endif
	images_per_cycle = (int)(360.0 / Moptop_Multrun_Rotator_Step_Angle_Get());
	/* acquire frames */
	for(Multrun_Data.Image_Index=0;Multrun_Data.Image_Index < Multrun_Data.Image_Count; Multrun_Data.Image_Index++)
	{
		/* get exposure start timestamp */
		clock_gettime(CLOCK_REALTIME,&(Multrun_Data.Exposure_Start_Time));
		/* If this is the first exposure in the multrun, 
		** the exposure start time is also the multrun start time. */
		if(Multrun_Data.Image_Index == 0)
			Multrun_Data.Multrun_Start_Time = Multrun_Data.Exposure_Start_Time;
		rotator_start_angle = fmod(requested_rotator_angle, 360.0);
		Multrun_Data.Rotation_Number = (Multrun_Data.Image_Index / images_per_cycle) + 1;
		Multrun_Data.Sequence_Number = (Multrun_Data.Image_Index % images_per_cycle) + 1;
		/* get an acquired image buffer */
		if(!CCD_Command_Grabber_Acquire_Image_Async_Wait_Timeout(CCD_Buffer_Get_Image_Buffer(),timeout_ms))
		{
			Moptop_General_Error_Number = 611;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to retrieve image buffer.");
			return FALSE;
		}
		if(Moptop_Config_Rotator_Is_Enabled())
		{
			/* get final rotator angle */
			if(!PIROT_Command_Query_POS(&current_rotator_position))
			{
				Moptop_General_Error_Number = 612;
				sprintf(Moptop_General_Error_String,
					"Multrun_Acquire_Images:Failed to query rotator position.");
				return FALSE;
			}
			rotator_difference = current_rotator_position-requested_rotator_angle;
			rotator_end_angle = fmod(current_rotator_position, 360.0);
		}
		else
		{
			/* emulate what the rotator angle should be */
			rotator_difference = Moptop_Multrun_Rotator_Step_Angle_Get();
			rotator_end_angle = fmod(requested_rotator_angle+Moptop_Multrun_Rotator_Step_Angle_Get(),360.0);
		}
		/* get camera image number */
		if(!CCD_Command_Get_Image_Number_From_Metadata(CCD_Buffer_Get_Image_Buffer(),
							       CCD_Setup_Get_Image_Size_Bytes(),&camera_image_number))
		{
			Moptop_General_Error_Number = 614;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:"
				"Failed to get image_number from metadata.");
			return FALSE;
		}
		/* get camera timestamp */
		if(!CCD_Command_Get_Timestamp_From_Metadata(CCD_Buffer_Get_Image_Buffer(),
							    CCD_Setup_Get_Image_Size_Bytes(),&camera_timestamp))
		{
			Moptop_General_Error_Number = 622;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:"
				"Failed to get timestamp from metadata.");
			return FALSE;
		}
		/* get exposure end timestamp */
		clock_gettime(CLOCK_REALTIME,&exposure_end_time);
		/* generate a new filename for this FITS image */
		Multrun_Get_Fits_Filename(images_per_cycle,do_standard,filename,MULTRUN_FITS_FILENAME_LENGTH);
		/* write fits image */
		Multrun_Write_Fits_Image(do_standard,pco_exposure_length_s,exposure_end_time,
					 camera_image_number,camera_timestamp,
					 requested_rotator_angle,rotator_start_angle,rotator_end_angle,
					 rotator_difference,
					 CCD_Buffer_Get_Image_Buffer(),CCD_Setup_Get_Image_Size_Bytes(),filename);
		/* add fits image to list */
		if(!CCD_Fits_Filename_List_Add(filename,filename_list,filename_count))
		{
			Moptop_General_Error_Number = 623;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:"
				"Failed to add filename '%s' to list of filenames (count = %d).",
				filename,(*filename_count));
			return FALSE;
		}
		/* increment theoretical start rotator angle of next exposure */
		requested_rotator_angle += Moptop_Multrun_Rotator_Step_Angle_Get();
		/* check for abort */
		if(Moptop_Abort)
		{
			Moptop_General_Error_Number = 613;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Multrun Aborted.");
			return FALSE;
		}
	}/* end for on Multrun_Data.Image_Index / Multrun_Data.Image_Count */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("multrun","moptop_multrun.c","Multrun_Acquire_Images",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","finished.");
#endif
	return TRUE;
}

/**
 * Generate the next FITS filename to write image data into.
 * <ul>
 * <li>Increment the window number by calling CCD_Fits_Filename_Next_Window.
 * <li>If the current window number (CCD_Fits_Filename_Window_Get) is greater than images_per_cycle, we must
 *     have started another rotation of the rotator.
 *     <ul>
 *     <li>Increment the run number (rotation number) by calling CCD_Fits_Filename_Next_Run. This resets the
 *         window number to zero.
 *     <li>Increment the window number back to one using CCD_Fits_Filename_Next_Window.
 *     </ul>
 * <li>Generate an unreduced FITS filename by calling CCD_Fits_Filename_Get_Filename.
 * </ul>
 * @param images_per_cycle The number of images we generate for a full rotation of the rotator.
 *        If the incremented window number is greater than this number, we reset the window number to one
 *        and increment the run number (rotation number).
 * @param do_standard A boolean, if TRUE this is an observation of a standard, otherwise it is not.
 * @param filename A previously allocated string to write the generated FITS image filename into.
 * @param filename_length The length of the filename buffer to store a filename in characters.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Window
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Run
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Get_Filename
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Window_Get
 */
static int Multrun_Get_Fits_Filename(int images_per_cycle,int do_standard,char *filename,int filename_length)
{
	enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type;

	/* increment the window number */
	CCD_Fits_Filename_Next_Window();
	/* if the window number is greater than the number of images we generate for a full rotation of the rotator,
	** we reset the window number to one and increment the run number (rotation number).*/
	if(CCD_Fits_Filename_Window_Get() > images_per_cycle)
	{
		CCD_Fits_Filename_Next_Run();
		CCD_Fits_Filename_Next_Window();
	}
	if(do_standard)
		exposure_type = CCD_FITS_FILENAME_EXPOSURE_TYPE_STANDARD;
	else
		exposure_type = CCD_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE;
	if(!CCD_Fits_Filename_Get_Filename(exposure_type,CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED,
					   filename,filename_length))
	{
		Moptop_General_Error_Number = 625;
		sprintf(Moptop_General_Error_String,"Multrun_Get_Fits_Filename:Getting filename failed.");
		return FALSE;		
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Get_Fits_Filename",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","New filename generated: '%s'.",filename);
#endif
	return TRUE;
}

/**
 * Write the FITS image to disk.
 * <ul>
 * <li>We set the "OBSTYPE" FITS keyword value based on the value of do_standard.
 * <li>We set the "FILTER1" FITS keyword value based on the cached filter name in Multrun_Data.Filter_Name.
 * <li>We set the "FILTERI1" FITS keyword value based on the cached filter name in Multrun_Data.Filter_Id.
 * <li>We set the "DATE"/"DATE-OBS"/"UTSTART" and "MJD" keyword values based on the value of Multrun_Data.Exposure_Start_Time.
 * <li>We set the "DATE-END" and "UTEND" keyword values based on the value of exposure_end_time.
 * <li>We set the "TELAPSE" keyword value based on the time elapsed between 
 *     Multrun_Data.Multrun_Start_Time and exposure_end_time.
 * <li>We set the "RUNNUM" keyword value to Multrun_Data.Rotation_Number.
 * <li>We set the "EXPNUM" keyword value to Multrun_Data.Sequence_Number.
 * <li>We set the "EXPTOTAL" keyword value to Multrun_Data.Image_Count.
 * <li>We set the "CCDXBIN"/"CCDYBIN" FITS keyword values based on CCD_Setup_Get_Binning. 
 * <li>We set the "CCDATEMP" FITS keyword value based on the cached CCD temperature stored in Multrun_Data.CCD_Temperature.
 * <li>We set the "TEMPSTAT" FITS keyword value based on the cached CCD temperature status stored in
 *     Multrun_Data.CCD_Temperature_Status_String.
 * <li>We set the "MOPRMODE" FITS keyword value to the requested rotator speed stored in Multrun_Data.Rotator_Speed.
 * <li>We set the "MOPRRATE" FITS keyword value to the rotator angular velocity stored in Multrun_Data.Rotator_Run_Velocity.
 * <li>We set the "MOPRREQ" FITS keyword value to the requested_rotator_angle.
 * <li>We set the "MOPRBEG" FITS keyword value to the rotator_start_angle.
 * <li>We set the "MOPREND" FITS keyword value to the rotator_end_angle.
 * <li>We set the "MOPRARC" FITS keyword value to the rotator_difference.
 * <li>We set the "MOPRNUM" FITS keyword value to the Multrun_Data.Rotation_Number.
 * <li>We set the "MOPRPOS" FITS keyword value to the Multrun_Data.Sequence_Number.
 * <li>We set the "EXPTIME" and "XPOSURE" FITS keyword value to the pco_exposure_length_s in seconds.
 * <li>We set the "EXPREQST" FITS keyword value to the Multrun_Data.Requested_Exposure_Length.
 * <li>We set the "CCDXPIXE" FITS keyword value to CCD_Setup_Get_Pixel_Width in m.
 * <li>We set the "CCDYPIXE" FITS keyword value to CCD_Setup_Get_Pixel_Height in m.
 * <li>We set the "PICNUM" FITS keyword value to the camera_image_number.
 * <li>We set the "CAMTIME" FITS keyword value to the camera_timestamp.
 * <li>We create a file lock on the filename to write to using CCD_Fits_Filename_Lock.
 * <li>We create the FITS filename using fits_create_file.
 * <li>We calculate the binned image dimensions using CCD_Setup_Get_Sensor_Width / CCD_Setup_Get_Sensor_Height / 
 *     CCD_Setup_Get_Binning.
 * <li>We create an empty image of the correct dimensions using fits_create_img.
 * <li>We write the FITS headers to the FITS image using CCD_Fits_Header_Write_To_Fits.
 * <li>We check the computed binned image size is not larger than the image_buffer_length.
 * <li>If Multrun_Data.Flip_X is TRUE, we call Moptop_Multrun_Flip_X to flip the image data in the X direction.
 * <li>If Multrun_Data.Flip_Y is TRUE, we call Moptop_Multrun_Flip_Y to flip the image data in the Y direction.
 * <li>We write the image data to the FITS image using fits_write_img.
 * <li>If the binning value is not 1, we retrieve the current CCDSCALE value, scale it by the binning, and update the FITS
 *     keyword value.
 * <li>We close the FITS image using fits_close_file.
 * <li>We remove the file lock on the FITS image using CCD_Fits_Filename_UnLock.
 * </ul>
 * @param do_standard A boolean, if TRUE this is an observation of a standard, otherwise it is not.
 * @param pco_exposure_length_s The exposure length as retrieved from the camera, in seconds.
 * @param exposure_end_time A timestamp representing the end time of the exposure being saved. Actually a timestamp
 *        taken just after the camera has signalled a buffer is available.
 * @param camera_image_number The camera image number extracted from the read-out image's meta-data.
 * @param camera_timestamp A timestamp recorded by the camera and extracted from the read-out image's meta-data.
 * @param requested_rotator_angle The requested rotator angle at which we expect the exposure to have started at 
 *        in degrees.
 * @param rotator_start_angle The rotator angle _in the current rotation_ at which 
 *        we expect the exposure to have started at in degrees.
 * @param rotator_end_angle The rotator angle _in the current rotation_ either measured or predicted at the end of the 
 *        exposure.
 * @param rotator_difference The difference between the rotator start angle and the rotator end angle in degrees.
 * @param image_buffer The image buffer containing the data to write to disk.
 * @param image_buffer_length The length of data in the image buffer.
 * @param filename A string containing the FITS filename to write the data into.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see #Moptop_Multrun_Flip_X
 * @see #Moptop_Multrun_Flip_Y
 * @see moptop_fits_header.html#Moptop_Fits_Header_String_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Integer_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Long_Long_Integer_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Float_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_TimeSpec_To_Date_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_TimeSpec_To_Date_Obs_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_TimeSpec_To_UtStart_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_TimeSpec_To_Mjd
 * @see moptop_general.html#fdifftime
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_METRE_MICROMETRE
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Lock
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_UnLock
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Width
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Height
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Binning
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Width
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Height
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Timestamp_Clock_Frequency
 */
static int Multrun_Write_Fits_Image(int do_standard,double pco_exposure_length_s,
				    struct timespec exposure_end_time,int camera_image_number,
				    struct timespec camera_timestamp,
				    double requested_rotator_angle,double rotator_start_angle,
				    double rotator_end_angle,double rotator_difference,
				    unsigned char *image_buffer,
				    int image_buffer_length,char *filename)
{
	fitsfile *fp = NULL;
	char exposure_time_string[64];
	double mjd,ccdscale,dvalue;
	long axes[2];
	int retval=0,status=0;
	int ncols_unbinned,nrows_unbinned,binning,ncols_binned,nrows_binned,ivalue;
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","Started saving FITS filename '%s'.",filename);
#endif
	/* sort out some FITS headers for later */
	/* OBSTYPE */
	if(do_standard)
		retval = Moptop_Fits_Header_String_Add("OBSTYPE","STANDARD",NULL);
	else
		retval = Moptop_Fits_Header_String_Add("OBSTYPE","EXPOSE",NULL);
	if(retval == FALSE)
		return FALSE;
	/* FILTER1 */
	if(!Moptop_Fits_Header_String_Add("FILTER1",Multrun_Data.Filter_Name,NULL))
		return FALSE;
	/* FILTERI1 */
	if(!Moptop_Fits_Header_String_Add("FILTERI1",Multrun_Data.Filter_Id,NULL))
		return FALSE;
	/* update DATE keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_Date_String(Multrun_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("DATE",exposure_time_string,"[UTC] Start date of obs."))
		return FALSE;
	/* update DATE-OBS keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_Date_Obs_String(Multrun_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("DATE-OBS",exposure_time_string,"[UTC] Start of obs."))
		return FALSE;
	/* update UTSTART keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_UtStart_String(Multrun_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("UTSTART",exposure_time_string,"[UTC] Start time of obs."))
		return FALSE;
	/* update MJD keyword from Multrun_Data.Exposure_Start_Time */
	/* note leap second correction not implemented yet (always FALSE). */
	if(!Moptop_Fits_Header_TimeSpec_To_Mjd(Multrun_Data.Exposure_Start_Time,FALSE,&mjd))
		return FALSE;
	if(!Moptop_Fits_Header_Float_Add("MJD",mjd,NULL))
		return FALSE;
	/* update DATE-END keyword from exposure_end_time */
	Moptop_Fits_Header_TimeSpec_To_Date_Obs_String(exposure_end_time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("DATE-END",exposure_time_string,"[UTC] End of obs."))
		return FALSE;
	/* update UTENDT keyword from exposure_end_time */
	Moptop_Fits_Header_TimeSpec_To_UtStart_String(exposure_end_time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("UTEND",exposure_time_string,"[UTC] End time of obs."))
		return FALSE;
	/* update TELAPSE keyword with difference between the multrun start timestamp and the 
	** end timestamp of the current exposure. This means TELAPSE is increasing in each frame of the multrun. */
	dvalue = fdifftime(exposure_end_time,Multrun_Data.Multrun_Start_Time);
	if(!Moptop_Fits_Header_Float_Add("TELAPSE",dvalue,"[sec] Total obs. duration"))
		return FALSE;
	/* update RUNNUM keyword value with the Multrun_Data.Rotation_Number */
	if(!Moptop_Fits_Header_Integer_Add("RUNNUM",Multrun_Data.Rotation_Number,
					   "Which rotation of the rotator we are on."))
		return FALSE;
	/* update EXPNUM keyword value with the Multrun_Data.Sequence_Number */
	if(!Moptop_Fits_Header_Integer_Add("EXPNUM",Multrun_Data.Sequence_Number,
					   "Which image in the current rotation we are on."))
		return FALSE;
	/* update EXPTOTAL keyword value with Multrun_Data.Image_Count */
	if(!Moptop_Fits_Header_Integer_Add("EXPTOTAL",Multrun_Data.Image_Count,
					   "Total number of frames expected this multrun."))
		return FALSE;
	/* ccdxbin */
	ivalue = CCD_Setup_Get_Binning();
	if(!Moptop_Fits_Header_Integer_Add("CCDXBIN",ivalue,NULL))
		return FALSE;
	/* ccdybin */
	ivalue = CCD_Setup_Get_Binning();
	if(!Moptop_Fits_Header_Integer_Add("CCDYBIN",ivalue,NULL))
		return FALSE;
	/* update actual ccd temperature with value stored at start of multrun */
	if(!Moptop_Fits_Header_Float_Add("CCDATEMP",Multrun_Data.CCD_Temperature,NULL))
		return FALSE;
	/* update ccd temperature status string with value stored at start of multrun */
	if(!Moptop_Fits_Header_String_Add("TEMPSTAT",Multrun_Data.CCD_Temperature_Status_String,NULL))
		return FALSE;
	/* update MOPRMODE with configured rotator speed string */
	if(!Moptop_Fits_Header_String_Add("MOPRMODE",Multrun_Data.Rotator_Speed,"Configured rotator speed"))
		return FALSE;
	/* update MOPRRATE with stored angular velocity of rotor in deg/sec */
	if(!Moptop_Fits_Header_Float_Add("MOPRRATE",Multrun_Data.Rotator_Run_Velocity,
					 "[deg/s] Angular velocity of rotator"))
		return FALSE;
	/* update MOPRREQ with requested_rotator_angle */
	if(!Moptop_Fits_Header_Float_Add("MOPRREQ",requested_rotator_angle,"[deg] MOPTOP Rotator requested angle"))
		return FALSE;
	/* update MOPRBEG with rotator_start_angle */
	if(!Moptop_Fits_Header_Float_Add("MOPRBEG",rotator_start_angle,"[deg] MOPTOP Rotator begin angle"))
		return FALSE;
	/* update MOPREND with rotator_end_angle */
	if(!Moptop_Fits_Header_Float_Add("MOPREND",rotator_end_angle,"[deg] MOPTOP Rotator angle at end of exposure"))
		return FALSE;
	/* update MOPRARC with rotator_difference */
	if(!Moptop_Fits_Header_Float_Add("MOPRARC",rotator_difference,"[deg] MOPTOP Rotator exposure arc"))
		return FALSE;
	/* MOPRNUM is the Multrun_Data.Rotation_Number */
	if(!Moptop_Fits_Header_Integer_Add("MOPRNUM",Multrun_Data.Rotation_Number,"MOPTOP Rotation number"))
		return FALSE;
	/* MOPRPOS is the Multrun_Data.Sequence_Number */
	if(!Moptop_Fits_Header_Integer_Add("MOPRPOS",Multrun_Data.Sequence_Number,"MOPTOP Position number within rotation"))
		return FALSE;
	/* EXPTIME is the actual exposure length returned from the camera, in seconds */
	if(!Moptop_Fits_Header_Float_Add("EXPTIME",pco_exposure_length_s,"[sec] Actual exposure"))
		return FALSE;
	/* XPOSURE is the actual exposure length returned from the camera, in seconds */
	if(!Moptop_Fits_Header_Float_Add("XPOSURE",pco_exposure_length_s,"[sec] Actual exposure"))
		return FALSE;
	/* EXPREQST is the requested exposure length in seconds (from Multrun_Data.Requested_Exposure_Length) */
	if(!Moptop_Fits_Header_Float_Add("EXPREQST",Multrun_Data.Requested_Exposure_Length,"[sec] Requested exposure"))
		return FALSE;
	/* CCDXPIXE is the physical x width of a pixel in metres */
	if(!Moptop_Fits_Header_Float_Add("CCDXPIXE",
					 CCD_Setup_Get_Pixel_Width()/((double)MOPTOP_GENERAL_ONE_METRE_MICROMETRE),
					 "[m] Detector pixel width"))
		return FALSE;
	/* CCDYPIXE is the physical y height of a pixel in metres */
	if(!Moptop_Fits_Header_Float_Add("CCDYPIXE",
					 CCD_Setup_Get_Pixel_Height()/((double)MOPTOP_GENERAL_ONE_METRE_MICROMETRE),
					 "[m] Detector pixel height"))
		return FALSE;
	/* PICNUM is the camera image number retrieved from the camera read out's metadata. */
	if(!Moptop_Fits_Header_Integer_Add("PICNUM",camera_image_number,"Camera meta-data image number"))
		return FALSE;
	/* CAMTIME is the camera clock timestamp the when this exposure started, from the camera's meta-data */
	Moptop_Fits_Header_TimeSpec_To_Date_Obs_String(camera_timestamp,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("CAMTIME",exposure_time_string,"[UTC] Cameras timestamp."))
		return FALSE;
	/* create lock file */
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","Locking FITS filename %s.",filename);
#endif
	if(!CCD_Fits_Filename_Lock(filename))
	{
		Moptop_General_Error_Number = 630;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:Failed to lock '%s'.",filename);
		return FALSE;				
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","Saving to filename %s.",filename);
#endif
	/* create FITS file */
	retval = fits_create_file(&fp,filename,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 631;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:File create failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* basic dimensions */
	ncols_unbinned = CCD_Setup_Get_Sensor_Width();
	nrows_unbinned = CCD_Setup_Get_Sensor_Height();
	binning = CCD_Setup_Get_Binning();
	ncols_binned = ncols_unbinned/binning;
	nrows_binned = nrows_unbinned/binning;
	axes[0] = ncols_binned;
	axes[1] = nrows_binned;
	retval = fits_create_img(fp,USHORT_IMG,2,axes,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 632;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:create image failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* save FITS headers to filename */
	if(!CCD_Fits_Header_Write_To_Fits(fp))
	{
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 633;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:CCD_Fits_Header_Write_To_Fits failed.");
		return FALSE;
	}
	/* save data to filename */
	if((ncols_binned*nrows_binned) > image_buffer_length)
	{
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 634;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:FITS image dimension mismatch:"
			"filename '%s', binned ncols = %d, binned_nrows = %d, image buffer length = %d.",
			filename,ncols_binned,nrows_binned,image_buffer_length);
		return FALSE;
	}
	/* check and flip images if configured to do so */
	if(Multrun_Data.Flip_X)
		Moptop_Multrun_Flip_X(ncols_binned,nrows_binned,(unsigned short *)image_buffer);
	if(Multrun_Data.Flip_Y)
		Moptop_Multrun_Flip_Y(ncols_binned,nrows_binned,(unsigned short *)image_buffer);
	/* write the data */
	retval = fits_write_img(fp,TUSHORT,1,ncols_binned*nrows_binned,image_buffer,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 635;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:File write failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* CCDSCALE */
	/* bin1 value configured in Java layer and passed into fits header list.
	** Should have been written to file in CCD_Fits_Header_Write_To_Fits.
	** So retrieve bin1 value using CFITSIO, mod by binning and update value */
	if(binning != 1)
	{
		retval = fits_read_key(fp,TDOUBLE,"CCDSCALE",&ccdscale,NULL,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fits_close_file(fp,&status);
			CCD_Fits_Filename_UnLock(filename);
			Moptop_General_Error_Number = 636;
			sprintf(Moptop_General_Error_String,
				"Multrun_Write_Fits_Image: Retrieving ccdscale failed(%s,%d,%s).",
				filename,status,buff);
			return FALSE;
		}
		/* adjust for binning */
		ccdscale *= binning; /* assume xbin and ybin are equal */
		/* write adjusted value back */
		retval = fits_update_key_fixdbl(fp,"CCDSCALE",ccdscale,6,NULL,&status);
		if(retval)
		{
			fits_get_errstatus(status,buff);
			fits_report_error(stderr,status);
			fits_close_file(fp,&status);
			CCD_Fits_Filename_UnLock(filename);
			Moptop_General_Error_Number = 637;
			sprintf(Moptop_General_Error_String,
				"Multrun_Write_Fits_Image: Updating ccdscale failed(%.2f,%s,%d,%s).",
				ccdscale,filename,status,buff);
			return FALSE;
		}
	}/* end if binning != 1 */
/* close file */
	retval = fits_close_file(fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 638;
		sprintf(Moptop_General_Error_String,
			"Multrun_Write_Fits_Image: File close failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	/* unlock FITS filename lock */
	if(!CCD_Fits_Filename_UnLock(filename))
	{
		Moptop_General_Error_Number = 639;
		sprintf(Moptop_General_Error_String,"Multrun_Write_Fits_Image:Failed to unlock '%s'.",filename);
		return FALSE;
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("multrun","moptop_multrun.c","Multrun_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,"MULTRUN",
			   "Finished.");
#endif
	return TRUE;
}


