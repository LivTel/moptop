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

/* data types */
/**
 * Data type holding local data to moptop multruns.
 * <dl>
 * <dt>Rotator_Run_Velocity</dt> <dd>A copy of the rotator run velocity used to configure the rotator.</dd>
 * <dt>Rotator_Step_Angle</dt> <dd>A copy of the rotator step angle used to configure the rotator.</dd>
 * <dt>Filter_Position;</dt> <dd>A copy of the filter position taken at the start of the multrun, used for filling in FITS headers.</dd>
 * <dt>Filter_Name</dt> <dd>A copy of the filter name taken at the start of the multrun, used for filling in FITS headers. Of length MULTRUN_FILTER_NAME_LENGTH.</dd>
 * <dt>Filter_Id</dt> <dd>A copy of the filter Id taken at the start of the multrun, used for filling in FITS headers. Of length MULTRUN_FILTER_NAME_LENGTH.</dd>
 * <dt>CCD_Temperature</dt> <dd>A copy of the current CCD temperature, taken at the start of a multrun. Used to populate FITS headers.</dd>
 * <dt>CCD_Temperature_Status_String</dt> <dd>A copy of the current CCD temperature status, taken at the start of a multrun. Used to populate FITS headers.</dd>
 * <dt>Requested_Exposure_Length</dt> <dd>A copy of the per-frame requested exposure length (in seconds) used to configure the CCD camera. 
 *                                     Used to populate FITS headers.</dd>
 * <dt>Image_Index</dt> <dd>Which frame in the multrun we are currently working on.</dd>
 * <dt>Image_Count</dt> <dd>The number of FITS images we are expecting to generate in the current multrun.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>A timestamp taken the last time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, the timestamp is only approximate).</dd>
 * <dt>Rotation_Number</dt> <dd>A number representing which rotation of the rotator we are on (from 1 to Multrun_Data.Image_Count/images_per_cycle).</dd>
 * <dt>Sequence_Number</dt> <dd>A number representing which image in the current rotation we are on (from 1 to images_per_cycle).</dd>
 * <dt></dt> <dd></dd>
 * <dt></dt> <dd></dd>
 * </dl>
 * @see #MULTRUN_FILTER_NAME_LENGTH
 */
struct Multrun_Struct
{
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
	struct timespec Exposure_Start_Time;
	int Rotation_Number;
	int Sequence_Number;
};
	
/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Multrun Data holding local data to moptop multruns.
 * <dl>
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
 * <dt>Exposure_Start_Time</dt>           <dd>{0,0}</dd>
 * <dt>Rotation_Number</dt>               <dd>0</dd>
 * <dt>Sequence_Number</dt>               <dd>0</dd>
 * <dt></dt> <dd></dd>
 * <dt></dt> <dd></dd>
 * <dt></dt> <dd></dd>
 * <dt></dt> <dd></dd>
 * </dl>
 * @see #Multrun_Struct
 */
static struct Multrun_Struct Multrun_Data =
{
	0.0,0.0,-1,"","",0.0,"",0.0,0,0,{0,0},0,0
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
static int Multrun_Write_Fits_Image(int do_standard,int andor_exposure_length_ms,struct timespec exposure_end_time,
				    long long int camera_ticks,double requested_rotator_angle,double rotator_start_angle,
				    double rotator_end_angle,double rotator_difference,
				    unsigned char *image_buffer,int image_buffer_length,char *filename);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to set the exposure length to be used for the Multrun. This is the exposure length _per-frame_, i.e. it depends
 * on the rotorspeed, and leads to allow enough time to readout and save each image before the rotator triggers the
 * start of the next exposure.
 * <ul>
 * <li>The camera exposure length is set using CCD_Exposure_Length_Set.
 * <li>The requested exposure length is stored in Multrun_Data.Requested_Exposure_Length (for later use in FITS headers).
 * </ul>
 * @param exposure_length_s The exposure length to use for each frame, in seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 * @see #Multrun_Data
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_command.html#CCD_Exposure_Length_Set
 */
int Moptop_Multrun_Exposure_Length_Set(double exposure_length_s)
{
	/* configure the CCD camera exposure length 
	** Note this might be modified by the Andor library. 
	** Note CCD_Exposure_Length_Set requires the exposure length to be in milliseconds. */
	if(!CCD_Exposure_Length_Set(exposure_length_s*MOPTOP_GENERAL_ONE_SECOND_MS))
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
 * Routine to setup the Multrun.
 * <ul>
 * <li>Check the rotator is in the correct start position using PIROT_Setup_Is_Rotator_At_Start_Position, 
 *     if the rotator is enabled.
 * <li>Increment the FITS filename multrun number and return it.
 * <li>Increment the FITS filename run number.
 * <li>If the filter wheel is enabled, we get the current filter wheel position using Filter_Wheel_Command_Get_Position.
 * <li>If the filter wheel is enabled, We get the current filter name using Filter_Wheel_Config_Position_To_Name.
 * <li>We get the filter id associated with the filter name (either previously cached or just retrieved) by calling
 *     Filter_Wheel_Config_Name_To_Id.
 * <li>We get and cache the current CCD temperature using CCD_Temperature_Get to store the temperature in 
 *     Multrun_Data.CCD_Temperature.
 * <li>We get and cache the current CCD temperature status string using CCD_Temperature_Get_Temperature_Status_String 
 *     to store the temperature in Multrun_Data.CCD_Temperature_Status_String.
 * </ul>
 * @param multrun_number The address of an integer to store the multrun number we expect to use for this multrun.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_config.html#Moptop_Config_Filter_Wheel_Is_Enabled
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Multrun
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Run
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Is_Rotator_At_Start_Position
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Position_To_Name
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Name_To_Id
 */
int Moptop_Multrun_Setup(int *multrun_number)
{
	if(multrun_number == NULL)
	{
		Moptop_General_Error_Number = 604;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: multrun_number was NULL.");
		return FALSE;
	}
	/* if the rotator is enabled, check it is in the right start position */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		if(!PIROT_Setup_Is_Rotator_At_Start_Position())
		{
			Moptop_General_Error_Number = 605;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun_Setup: Rotator not at start position.");
			return FALSE;
		}
	}
	/* increment the multrun number */
	CCD_Fits_Filename_Next_Multrun();
	(*multrun_number) = CCD_Fits_Filename_Multrun_Get();
	/* increment the run number (effectively which rotation we are on) to one */
	CCD_Fits_Filename_Next_Run();
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
	** Moptop_Multrun_Filter_Name_Set only gets called on C layers that have no filter wheel (moptop2).
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
 * <li>We queue the image buffers for the acquisitions using CCD_Buffer_Queue_Images.
 * <li>We reset the cameras internal timestamp clock using CCD_Command_Timestamp_Clock_Reset.
 * <li>We set the camera to trigger externally (from the rotator) using CCD_Command_Set_Trigger_Mode.
 * <li>We enable the camera for acquisitions using CCD_Command_Acquisition_Start.
 * <li>If the rotator is enabled (Moptop_Config_Rotator_Is_Enabled):
 *     <ul>
 *     <li>We enable the rotator hardware triggers using PIROT_Command_TRO.
 *     <li>We command the rotator to start moving towards it's end position using 
 *         PIROT_Command_MOV(rotator_end_position).
 *     </ul>
 * <li>We acquire the image date using  Multrun_Acquire_Images.
 * <li>We disable the camera for acquisition using CCD_Command_Acquisition_Stop.
 * <li>We set the camera to trigger from software using CCD_Command_Set_Trigger_Mode.
 * <li>We flush the camera (and associated queued image buffers) using CCD_Command_Flush.
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
 * @see # Multrun_Acquire_Images
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Step_Angle_Get
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Queue_Images
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_EXTERNAL
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Start
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Stop
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Flush
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Timestamp_Clock_Reset
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
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(Multrun_Data.Image_Count))
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 609;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to queue image buffers.");
		return FALSE;
	}
	/* reset internal clock timestamp */
	if(!CCD_Command_Timestamp_Clock_Reset())
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 614;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to reset camera timestamp clock.");
		return FALSE;
	}
	/* turn on camera external triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_EXTERNAL))
	{
		Multrun_In_Progress = FALSE;
		Moptop_General_Error_Number = 644;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to set camera trigger mode to external.");
		return FALSE;
	}		
	/* enable the CCD acquisition */
	if(!CCD_Command_Acquisition_Start())
	{
		Multrun_In_Progress = FALSE;
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		Moptop_General_Error_Number = 615;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to start camera acquisition.");
		return FALSE;
	}
	/* only configure and move the rotator, this this is the C layer with it enabled */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		/* enable rotator hardware trigger */
		if(!PIROT_Command_TRO(TRUE))
		{
			Multrun_In_Progress = FALSE;
			CCD_Command_Acquisition_Stop();
			CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
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
			CCD_Command_Acquisition_Stop();
			CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
			PIROT_Command_TRO(FALSE);
			Moptop_General_Error_Number = 607;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to move rotator to end position %.2f.",
				rotator_end_position);
			return FALSE;
		}
	}/* end if rotator enabled */
	/* acquire camera images */
	retval = Multrun_Acquire_Images(do_standard,filename_list,filename_count);
	if(retval == FALSE)
	{
		CCD_Command_Acquisition_Stop();
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		CCD_Command_Flush();
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Multrun_In_Progress = FALSE;
		return FALSE;
	}
	/* stop CCD acquisition */
	if(!CCD_Command_Acquisition_Stop())
	{
		Multrun_In_Progress = FALSE;
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Moptop_General_Error_Number = 616;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to stop camera acquisition.");
		return FALSE;
	}
	/* turn off external camera triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE))
	{
		Multrun_In_Progress = FALSE;
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Moptop_General_Error_Number = 645;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to set camera trigger mode to software.");
		return FALSE;
	}		
	if(!CCD_Command_Flush())
	{
		Multrun_In_Progress = FALSE;
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Moptop_General_Error_Number = 624;
		sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to flush camera.");
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
 *     <li>Stop camera acquisition using CCD_Command_Acquisition_Stop.
 *     <li>If therotator is enabled (Moptop_Config_Rotator_Is_Enabled), stop the rotator triggering using 
 *         PIROT_Command_TRO.
 *     </ul>
 * <li>Returns Multrun_In_Progress (i.e. whether there was a multrun in progress to be aborted).
 * </ul>
 * @return TRUE if there was a multrun in progress to be aborted, FALSE otherwise.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Stop
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_TRO
 */
int Moptop_Multrun_Abort(void)
{
	Moptop_Abort = TRUE;
	if(Multrun_In_Progress)
	{
		/* stop the camera acquisition */
		if(!CCD_Command_Acquisition_Stop())
		{
			Moptop_General_Error_Number = 617;
			sprintf(Moptop_General_Error_String,
				"Moptop_Multrun_Abort:Failed to stop camera acquisition.");
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
	return Multrun_In_Progress;
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
 * @param exposure_start_time The address of a timespec structure to fill with the  start time timestamp.
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
 * Return the window number (in the generated FITS filenames) of this multrun. This is the image within a rotation we are on.
 * @return The current run number (the image within a rotation).
 * @see #Multrun_Data
 */
int Moptop_Multrun_Window_Get(void)
{
	return Multrun_Data.Sequence_Number;	
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
/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to actually acquire the externally triggered images with the rotator moving.
 * <ul>
 * <li>We check the filename_list and filename_count are not NULL and initialise them.
 * <li>We get the camera exposure length using CCD_Exposure_Length_Get.
 * <li>We get the camera's internal clock frequency using CCD_Command_Get_Timestamp_Clock_Frequency.
 * <li>We calculate a timeout as being twice the length of time between two triggers.
 * <li>We initialise last_camera_ticks to zero.
 * <li>We loop over the Multrun_Data.Image_Count, using Multrun_Data.Image_Index as an index counter (for status reporting):
 *     <ul>
 *     <li>We take a timestamp and store it in Multrun_Data.Exposure_Start_Time.
 *     <li>We compute the theoretical rotator start angle (within a rotation) and store it in rotator_start_angle.
 *     <li>We compute which rotation we are on and store it in Multrun_Data.Rotation_Number.
 *     <li>We compute the image we are taking within the current rotation and store it in Multrun_Data.Sequence_Number.
 *     <li>We wait for a readout by calling CCD_Command_Wait_Buffer.
 *     <li>If the rotator is configured (Moptop_Config_Rotator_Is_Enabled) we retrieve the actual final rotator 
 *         position using PIROT_Command_Query_POS, and use it compute the rotator_difference and the
 *         rotator_end_angle (the curent position in the current rotation).
 *     <li>If the rotator is _not_ configured  we compute a theoretical rotator_difference and rotator_end_angle.
 *     <li>We get the camera image timestamp from the image metadata using CCD_Command_Get_Timestamp_From_Metadata.
 *     <li>We calculate the camera_clock_difference as the difference in camera ticks between this image and the 
 *         last one, divided by the previously retrieved camera's internal clock frequency.
 *     <li>We get an exposure end timestamp and store it in exposure_end_time.
 *     <li>We call Multrun_Get_Fits_Filename to generate a new FITS filename.
 *     <li>We call Multrun_Write_Fits_Image to write the image data to the generated FTYS filename.
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
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Wait_Buffer
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Timestamp_Clock_Frequency
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Timestamp_From_Metadata
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Add
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_Query_POS
 */
static int Multrun_Acquire_Images(int do_standard,char ***filename_list,int *filename_count)
{
	struct timespec exposure_end_time;
	char filename[MULTRUN_FITS_FILENAME_LENGTH];
	unsigned char *image_buffer = NULL;
	unsigned int timeout_ms;
	double requested_rotator_angle = 0.0;
	double rotator_start_angle,current_rotator_position,rotator_difference,rotator_end_angle;
	double camera_clock_difference;
	long long int camera_ticks,last_camera_ticks,timestamp_clock_frequency;
	int andor_exposure_length_ms;
	int image_buffer_length,images_per_cycle;
	
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
	/* compute how long CCD_Command_Wait_Buffer should wait for an image to arrive, in milliseconds */
	/* get exposure length used by andor */
	if(!CCD_Exposure_Length_Get(&andor_exposure_length_ms))
	{
		Moptop_General_Error_Number = 610;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to get Andor exposure length.");
		return FALSE;
	}
	/* get camera's internal clock frequency for use in the loop */
	if(!CCD_Command_Get_Timestamp_Clock_Frequency(&timestamp_clock_frequency))
	{
		Moptop_General_Error_Number = 621;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to get timestamp clock frequency.");
		return FALSE;
	}
	/* time taken between two triggers is rotator_step_angle/rotator_run_velocity 
	** Lets make it four times that in milliseconds, at two times the wait on moptop2 can timeout for the first frame
	** as it starts earlier than that on moptop1 (as moptop1 is also starting the rotator etc...) */
	timeout_ms = (int)(4.0*(Moptop_Multrun_Rotator_Step_Angle_Get()/Moptop_Multrun_Rotator_Run_Velocity_Get()))*
		MOPTOP_GENERAL_ONE_SECOND_MS;
	images_per_cycle = (int)(360.0 / Moptop_Multrun_Rotator_Step_Angle_Get());
	last_camera_ticks = 0;
	/* acquire frames */
	for(Multrun_Data.Image_Index=0;Multrun_Data.Image_Index < Multrun_Data.Image_Count; Multrun_Data.Image_Index++)
	{
		/* get exposure start timestamp */
		clock_gettime(CLOCK_REALTIME,&(Multrun_Data.Exposure_Start_Time));
		rotator_start_angle = fmod(requested_rotator_angle, 360.0);
		Multrun_Data.Rotation_Number = (Multrun_Data.Image_Index / images_per_cycle) + 1;
		Multrun_Data.Sequence_Number = (Multrun_Data.Image_Index % images_per_cycle) + 1;
		/* get an acquired image buffer */
		if(!CCD_Command_Wait_Buffer(&image_buffer,&image_buffer_length,timeout_ms))
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
		/* get camera timestamp */
		if(!CCD_Command_Get_Timestamp_From_Metadata(image_buffer,image_buffer_length,&camera_ticks))
		{
			Moptop_General_Error_Number = 622;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:"
				"Failed to get timestamp from metadata.");
			return FALSE;
		}
		camera_clock_difference = ((double)(camera_ticks-last_camera_ticks))/(double)timestamp_clock_frequency;
		last_camera_ticks = camera_ticks;
		/* get exposure end timestamp */
		clock_gettime(CLOCK_REALTIME,&exposure_end_time);
		/* generate a new filename for this FITS image */
		Multrun_Get_Fits_Filename(images_per_cycle,do_standard,filename,MULTRUN_FITS_FILENAME_LENGTH);
		/* write fits image */
		Multrun_Write_Fits_Image(do_standard,andor_exposure_length_ms,exposure_end_time,camera_ticks,
					 requested_rotator_angle,rotator_start_angle,rotator_end_angle,
					 rotator_difference,image_buffer,image_buffer_length,filename);
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
	/* get camera temperature */

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
 * <li>We set the "TELAPSE" keyword value based on the time elapsed between Multrun_Data.Exposure_Start_Time and exposure_end_time.
 * <li>We set the "RUNNUM" keyword value to Multrun_Data.Rotation_Number.
 * <li>We set the "EXPNUM" keyword value to Multrun_Data.Sequence_Number.
 * <li>We set the "EXPTOTAL" keyword value to Multrun_Data.Image_Count.
 * <li>We set the "CCDXBIN"/"CCDYBIN" FITS keyword values based on CCD_Setup_Get_Binning. 
 * <li>We set the "CCDATEMP" FITS keyword value based on the cached CCD temperature stored in Multrun_Data.CCD_Temperature.
 * <li>We set the "TEMPSTAT" FITS keyword value based on the cached CCD temperature status stored in
 *     Multrun_Data.CCD_Temperature_Status_String.
 * <li>We set the "MOPRREQ" FITS keyword value to the requested_rotator_angle.
 * <li>We set the "MOPRBEG" FITS keyword value to the rotator_start_angle.
 * <li>We set the "MOPREND" FITS keyword value to the rotator_end_angle.
 * <li>We set the "MOPRARC" FITS keyword value to the rotator_difference.
 * <li>We set the "MOPRNUM" FITS keyword value to the Multrun_Data.Rotation_Number.
 * <li>We set the "MOPRPOS" FITS keyword value to the Multrun_Data.Sequence_Number.
 * <li>We set the "EXPTIME" and "XPOSURE" FITS keyword value to the andor_exposure_length_ms in seconds.
 * <li>We set the "EXPREQST" FITS keyword value to the Multrun_Data.Requested_Exposure_Length.
 * <li>We set the "CCDXPIXE" FITS keyword value to CCD_Setup_Get_Pixel_Width in m.
 * <li>We set the "CCDYPIXE" FITS keyword value to CCD_Setup_Get_Pixel_Height in m.
 * <li>We set the "CLKFREQ" FITS keyword value to CCD_Setup_Get_Timestamp_Clock_Frequency.
 * <li>We set the "CLKSTAMP" FITS keyword value to camera_ticks.
 * <li>We create a file lock on the filename to write to using CCD_Fits_Filename_Lock.
 * <li>We create the FITS filename using fits_create_file.
 * <li>We calculate the binned image dimensions using CCD_Setup_Get_Sensor_Width / CCD_Setup_Get_Sensor_Height / 
 *     CCD_Setup_Get_Binning.
 * <li>We create an empty image of the correct dimensions using fits_create_img.
 * <li>We write the FITS headers to the FITS image using CCD_Fits_Header_Write_To_Fits.
 * <li>We check the computed binned image size is not larger than the image_buffer_length.
 * <li>We write the image data to the FITS image using fits_write_img.
 * <li>If the binning value is not 1, we retrieve the current CCDSCALE value, scale it by the binning, and update the FITS
 *     keyword value.
 * <li>We close the FITS image using fits_close_file.
 * <li>We remove the file lock on the FITS image using CCD_Fits_Filename_UnLock.
 * </ul>
 * @param do_standard A boolean, if TRUE this is an observation of a standard, otherwise it is not.
 * @param andor_exposure_length_ms The exposure length as retrieved from the camera, in milliseconds.
 * @param exposure_end_time A timestamp representing the end time of the exposure being saved. Actually a timestamp
 *        taken just after the camera has signalled a buffer is available.
 * @param camera_ticks A long long int holding the camera clock ticks stored by the camera in it's meta-data at the
 *        instant it began this exposure.
 * @param requested_rotator_angle The requested rotator angle at which we expect the exposure to have started at 
 *        in degrees.
 * @param rotator_start_angle The rotator angle _in the current rotation_ at which 
 *        we expect the exposure to have started at in degrees.
 * @param rotator_end_angle The rotator angle _in the current rotation_ either measured or predicted at the end of the 
 *        exposure.
 * @param rotator_difference The difference between the rotator start angle and the rotator end angle in degrees.
 * @param image_buffer The image buffer containing the data to write to disk.
 * @param image_buffer_length The length of data in the image buffer (note this includes metadata).
 * @param filename A string containing the FITS filename to write the data into.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
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
static int Multrun_Write_Fits_Image(int do_standard,int andor_exposure_length_ms,
				    struct timespec exposure_end_time,long long int camera_ticks,
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
	/* update TELAPSE keyword with difference between start and end timestamps */
	dvalue = fdifftime(exposure_end_time,Multrun_Data.Exposure_Start_Time);
	if(!Moptop_Fits_Header_Float_Add("TELAPSE",dvalue,"[sec] Total obs. duration"))
		return FALSE;
	/* update RUNNUM keyword value with the Multrun_Data.Rotation_Number */
	if(!Moptop_Fits_Header_Integer_Add("RUNNUM",Multrun_Data.Rotation_Number,"Which rotation of the rotator we are on."))
		return FALSE;
	/* update EXPNUM keyword value with the Multrun_Data.Sequence_Number */
	if(!Moptop_Fits_Header_Integer_Add("EXPNUM",Multrun_Data.Sequence_Number,"Which image in the current rotation we are on."))
		return FALSE;
	/* update EXPTOTAL keyword value with Multrun_Data.Image_Count */
	if(!Moptop_Fits_Header_Integer_Add("EXPTOTAL",Multrun_Data.Image_Count,"Total number of frames expected this multrun."))
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
	if(!Moptop_Fits_Header_Float_Add("EXPTIME",
					 ((double)andor_exposure_length_ms)/((double)MOPTOP_GENERAL_ONE_SECOND_MS),
					 "[sec] Actual exposure"))
		return FALSE;
	/* XPOSURE is the actual exposure length returned from the camera, in seconds */
	if(!Moptop_Fits_Header_Float_Add("XPOSURE",
					 ((double)andor_exposure_length_ms)/((double)MOPTOP_GENERAL_ONE_SECOND_MS),
					 "[sec] Actual exposure"))
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
	/* CLKFREQ is the timestamp clock frequency of the internal camera clock */
	if(!Moptop_Fits_Header_Long_Long_Integer_Add("CLKFREQ",CCD_Setup_Get_Timestamp_Clock_Frequency(),
						     "[Hz] Detector clock tick frequency"))
		return FALSE;
	/* CLKSTAMP is the camera clock timestamp (in ticks) the camera took when this exposure started */
	if(!Moptop_Fits_Header_Long_Long_Integer_Add("CLKSTAMP",camera_ticks,"Image clock tick value"))
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
		sprintf(Moptop_General_Error_String,"Multi_Exposure_Save:Failed to unlock '%s'.",filename);
		return FALSE;
	}
#if LOGGING > 0
	CCD_Global_Log("ccd","ccd_multi_exposure.c","Multi_Exposure_Save",LOG_VERBOSITY_INTERMEDIATE,"Multi_Exposure",
		       "Finished.");
#endif
	return TRUE;
}
