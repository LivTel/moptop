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

#include "pirot_command.h"
#include "pirot_setup.h"

#include "moptop_config.h"
#include "moptop_general.h"
#include "moptop_multrun.h"

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Is a multrun in progress.
 */
static int Multrun_In_Progress = FALSE;
/**
 * Abort any multrun currently in progress.
 */
static int Moptop_Abort = FALSE;
/**
 * A copy of the rotator run velocity used to configure the rotator.
 */
static double Multrun_Rotator_Run_Velocity;
/**
 * A copy of the rotator step angle used to configure the rotator.
 */
static double Multrun_Rotator_Step_Angle;

/* internal functions */
static int Multrun_Acquire_Images(int frame_count,char ***filename_list,int *filename_count);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to setup the Multrun.
 * <ul>
 * <li>Check the rotator is in the correct start position using PIROT_Setup_Is_Rotator_At_Start_Position, 
 *     if the rotator is enabled.
 * <li>Increment the FITS filename multrun number and return it.
 * <li>
 * </ul>
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Multrun
C * @see ../ccd/cdocs/ccd_fits_filename.html#CD_Fits_Filename_Multrun_Get
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Is_Rotator_At_Start_Position
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
 *     <li>Compute the number of rotations (rotation_count) = exposure_length_ms/(360.0/rotator_run_velocity)
 *     <li>Check the computed rotation_count is at least one.
 *     </ul>
 * <li>If use_exposure_count is set, we set the rotation_count to exposure_count.
 * <li>We retrieve the configured rotator_step_angle using Moptop_Multrun_Rotator_Step_Angle_Get.
 * <li>We compute the number of exposures (frame_count) = rotation_count*(360.0/trigger_step_angle).
 * <li>We compute the rotator_end_position = (360.0 * rotation_count)-PIROT_SETUP_ROTATOR_TOLERANCE. 
 *     We stop short on the last exposure to avoid an extra trigger/frame.
 * <li>If the rotator is enabled (Moptop_Config_Rotator_Is_Enabled):
 *     <ul>
 *     <li>We enable the rotator hardware triggers using PIROT_Command_TRO.
 *     <li>We command the rotator to start moving towards it's end position using PIROT_Command_MOV(rotator_end_position).
 *     </ul>
 * <li>We acquire the images...
 * <li>
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
 * @param filename_list The address of a list of filenames of FITS images acquired during this multrun.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the exposure succeeds, returns FALSE if an error occurs or the exposure is aborted.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Step_Angle_Get
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_TRO
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_SETUP_ROTATOR_TOLERANCE
 */
int Moptop_Multrun(int exposure_length_ms,int use_exposure_length,int exposure_count,int use_exposure_count,
		   char ***filename_list,int *filename_count)
{
	double rotator_run_velocity,trigger_step_angle,rotator_end_position;
	int retval,rotation_count,frame_count;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_TERSE,"MULTRUN",
				  "(exposure_length_ms = %d,use_exposure_length = %d,exposure_count = %d,"
				  "use_exposure_count = %d,filename_list = %p,filename_count = %p) started.",
				  exposure_length_ms,use_exposure_length,exposure_count,use_exposure_count,
				  filename_list,filename_count);
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
		rotator_run_velocity = Moptop_Multrun_Rotator_Run_Velocity_Get();
		rotation_count = exposure_length_ms/(360.0/rotator_run_velocity);
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
	/* how many exposures are we expecting */
	trigger_step_angle = Moptop_Multrun_Rotator_Step_Angle_Get();
	frame_count = rotation_count*(360.0/trigger_step_angle);
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
				  "We are expecting %d frames.",frame_count);
#endif
	/* what is the rotator end position? */
	/* stop short on last exposure to avoid an extra trigger/frame */
	rotator_end_position = (360.0 * rotation_count)-PIROT_SETUP_ROTATOR_TOLERANCE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
				  "Rotator end position %.3f.",rotator_end_position);
#endif
	/* only configure and move the rotator, this this is the C layer with it enabled */
	if(Moptop_Config_Rotator_Is_Enabled())
	{
		/* enable rotator hardware trigger */
		if(!PIROT_Command_TRO(TRUE))
		{
			Multrun_In_Progress = FALSE;
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
			Moptop_General_Error_Number = 607;
			sprintf(Moptop_General_Error_String,"Moptop_Multrun:Failed to move rotator to end position %.2f.",
			rotator_end_position);
			return FALSE;
		}
	}/* end if rotator enabled */
	/* acquire camera images */
	retval = Multrun_Acquire_Images(frame_count,filename_list,filename_count);
	if(retval == FALSE)
	{
		if(Moptop_Config_Rotator_Is_Enabled())
			PIROT_Command_TRO(FALSE);
		Multrun_In_Progress = FALSE;
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
 * Abort a currently running multrun. Sets Moptop_Abort to TRUE, and returns Multrun_In_Progress
 * (i.e. whether there was a multrun in progress to be aborted).
 * @return TRUE if there was a multrun in progress to be aborted, FALSE otherwise.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 */
int Moptop_Multrun_Abort(void)
{
	Moptop_Abort = TRUE;
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
 * Set the stored rotator run velocity (used for computations inside Multrun_Multrun).
 * @param velocity The rotator velocity configured, in degrees/s.
 * @see #Multrun_Rotator_Run_Velocity
 */
void Moptop_Multrun_Rotator_Run_Velocity_Set(double velocity)
{
	Multrun_Rotator_Run_Velocity = velocity;
}

/**
 * Set the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @param step_angle The rotator step angle configured, in degrees.
 * @see #Multrun_Rotator_Step_Angle
 */
void Moptop_Multrun_Rotator_Step_Angle_Set(double step_angle)
{
	Multrun_Rotator_Step_Angle = step_angle;
}

/**
 * Get the stored rotator run velocity (used for computations inside Multrun_Multrun).
 * @return The stored rotator velocity in degrees/s.
 * @see #Multrun_Rotator_Run_Velocity
 */
double Moptop_Multrun_Rotator_Run_Velocity_Get(void)
{
	return Multrun_Rotator_Run_Velocity;
}

/**
 * Get the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @return The stored rotator step angle, in degrees.
 * @see #Multrun_Rotator_Step_Angle
 */
double Moptop_Multrun_Rotator_Step_Angle_Get(void)
{
	return Multrun_Rotator_Step_Angle;
}
/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to actually acquire the externally triggered images with the rotator moving.
 * @param frame_count The number of frames to acquire.
 * @param filename_list The address of a list of filenames of FITS images acquired during this multrun.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Moptop_Abort
 * @see #Moptop_Multrun_Rotator_Step_Angle_Get
 * @see #Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Queue_Images
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Wait_Buffer
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Get
 */
static int Multrun_Acquire_Images(int frame_count,char ***filename_list,int *filename_count)
{
	struct timespec exposure_start_time;
	unsigned char *image_buffer = NULL;
	unsigned int timeout_ms;
	double requested_rotator_angle = 0.0;
	double rotator_start_angle,current_rotator_position,rotator_difference,rotator_end_angle;
	int andor_exposure_length_ms;
	int i,image_buffer_length,images_per_cycle,rotator_number,sequence_number;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("multrun","moptop_multrun.c","Multrun_Acquire_Images",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","started with frame_count %d.",frame_count);
#endif
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(frame_count))
	{
		Moptop_General_Error_Number = 609;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to queue image buffers.");
		return FALSE;
	}
	/* compute how long CCD_Command_Wait_Buffer should wait for an image to arrive, in milliseconds */
	/* get exposure length used by andor */
	if(!CCD_Exposure_Length_Get(&andor_exposure_length_ms))
	{
		Moptop_General_Error_Number = 610;
		sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Failed to get Andor exposure length.");
		return FALSE;
	}
	/* time taken between two triggers is rotator_step_angle/rotator_run_velocity 
	** Lets make it twice that in milliseconds */
	timeout_ms = (int)(2.0*(Moptop_Multrun_Rotator_Step_Angle_Get()/Moptop_Multrun_Rotator_Run_Velocity_Get()))*
		MOPTOP_GENERAL_ONE_SECOND_MS;
	images_per_cycle = (int)(360.0 / Moptop_Multrun_Rotator_Step_Angle_Get());
	/* acquire frames */
	for(i=0;i< frame_count; i++)
	{
		/* get exposure start timestamp */
		clock_gettime(CLOCK_REALTIME,&exposure_start_time);
		rotator_start_angle = fmod(requested_rotator_angle, 360.0);
		rotator_number = (i / images_per_cycle) + 1;
		sequence_number = (i % images_per_cycle) + 1;
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
			rotator_end_angle = fmod(requested_rotator_angle + Moptop_Multrun_Rotator_Step_Angle_Get(), 360.0);
		}
		/* get camera timestamp */

		/* write fits image */

		/* add fits image to list */
		
		/* increment theoretical start rotator angle of next exposure */
		requested_rotator_angle += Moptop_Multrun_Rotator_Step_Angle_Get();
		/* check for abort */
		if(Moptop_Abort)
		{
			/* diddly do something here! */
			
			Moptop_General_Error_Number = 613;
			sprintf(Moptop_General_Error_String,"Multrun_Acquire_Images:Multrun Aborted.");
			return FALSE;
		}
	}/* end for on i / frame_count */
	/* turn off camera acquisition */

	/* turn off camera triggering */

	/* get camera temperature */

	/* flush camera */
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("multrun","moptop_multrun.c","Multrun_Acquire_Images",LOG_VERBOSITY_INTERMEDIATE,
				  "MULTRUN","finished.");
#endif
	return TRUE;
}
