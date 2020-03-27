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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

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
static double Rotator_Run_Velocity;
/**
 * A copy of the rotator step angle used to configure the rotator.
 */
static double Rotator_Step_Angle;

/* internal functions */

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to actually do a multrun for one machine.
 * <ul>
 * <li>We initialise Moptop_Abort to FALSE, and Moptop_In_Progress to TRUE.
 * <li>
 * <li>We set Moptop_In_Progress to FALSE.
 * </ul>
 * @param exposure_length_ms The length of time to open the shutter for in milliseconds. 
 * @param use_exposure_length A boolean, if TRUE use the exposure length.
 * @param exposure_count The number of exposures to take. In this case, used to determine the number of complete
 *        rotations to use.
 * @param use_exposure_count A boolean, if TRUE use the exposure count.
 * @return Returns TRUE if the exposure succeeds, returns FALSE if an error occurs or the exposure is aborted.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_SETUP_ROTATOR_TOLERANCE
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Rotator_Run_Velocity_Get
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Trigger_Step_Angle_Get
 */
int Moptop_Multrun(int exposure_length_ms,int use_exposure_length,int exposure_count,int use_exposure_count,
		   char ***filename_list,int *filename_count)
{
	double rotator_run_velocity,trigger_step_angle,rotator_end_point;
	int rotation_count,frame_count;
	
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
		/* diddly this won't work on moptop2 */
		rotator_run_velocity = Moptop_Multrun_Rotator_Run_Velocity_Get();
		rotation_count = exposure_length_ms/(360.0/rotator_run_velocity);
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_VERBOSE,"MULTRUN",
			  "Using exposure length %d ms, rotator run velocity %.2f deg/s, therefore rotation count %d.",
					  exposure_length_ms,rotator_run_velocity,rotation_count);
#endif
		if(rotation_count < 1)
		{
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
	/* how many exposures are we expecting */
	/* diddly this won't work on moptop2 */
	trigger_step_angle = PIROT_Setup_Trigger_Step_Angle_Get();
	frame_count = rotation_count*(360.0/trigger_step_angle);
	/* what is the rotator end point? */
	/* stop short on last exposure to avaoid an extra trigger/frame */
	rotator_end_point = (360.0 * rotation_count)-PIROT_SETUP_ROTATOR_TOLERANCE;
	/* enable camera hardware trigger */
	
	/* move rotator to it's final position */
	
	/* acquire camera images */
	
	/* disable camera hardware trigger */
	
	Multrun_In_Progress = FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("multrun","moptop_multrun.c","Moptop_Multrun",LOG_VERBOSITY_TERSE,"MULTRUN","finished.");
#endif
	return TRUE;
}

/**
 * Abort a currently running multrun. Sets Moptop_Abort to TRUE, and returns Multrun_In_Progress
 * (i.e. whether there was a multrun in progress to be aborted.
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
 * @see #Rotator_Run_Velocity
 */
void Moptop_Multrun_Rotator_Run_Velocity_Set(double velocity)
{
	Rotator_Run_Velocity = velocity;
}

/**
 * Set the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @param step_angle The rotator step angle configured, in degrees.
 * @see #Rotator_Step_Angle
 */
void Moptop_Multrun_Rotator_Step_Angle_Set(double step_angle)
{
	Rotator_Step_Angle = step_angle;
}

/**
 * Get the stored rotator run velocity (used for computations inside Multrun_Multrun).
 * @return The stored rotator velocity in degrees/s.
 * @see #Rotator_Run_Velocity
 */
double Moptop_Multrun_Rotator_Run_Velocity_Get(void)
{
	return Rotator_Run_Velocity;
}

/**
 * Get the stored rotator step angle (used for computations inside Multrun_Multrun).
 * @return The stored rotator step angle, in degrees.
 * @see #Rotator_Step_Angle
 */
double Moptop_Multrun_Rotator_Step_Angle_Get(void)
{
	return Rotator_Step_Angle;
}
