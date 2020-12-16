/* moptop_bias_dark.c
** Moptop bias and dark routines
*/
/**
 * Bias and Dark routines for the moptop program.
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

#include "moptop_config.h"
#include "moptop_fits_header.h"
#include "moptop_general.h"
#include "moptop_multrun.h"

/* hash defines */
/**
 * Length of FITS filename string.
 */
#define BIAS_DARK_FITS_FILENAME_LENGTH  (256)

/* data types */
/**
 * Data type holding local data to moptop bias and darks.
 * <dl>
 * <dt>CCD_Temperature</dt> <dd>A copy of the current CCD temperature, taken at the start of a multrun. 
 *                              Used to populate FITS headers.</dd>
 * <dt>CCD_Temperature_Status_String</dt> <dd>A copy of the current CCD temperature status, 
 *                                            taken at the start of a multrun. Used to populate FITS headers.</dd>
 * <dt>Requested_Exposure_Length</dt> <dd>A copy of the requested exposure length (in seconds) 
 *                                        used to configure the CCD camera. Used to populate FITS headers.</dd>
 * <dt>Image_Index</dt> <dd>Which frame in the multrun we are currently working on.</dd>
 * <dt>Image_Count</dt> <dd>The number of FITS images we are expecting to generate in the current multrun.</dd>
 * <dt>Multrun_Start_Time</dt> <dd>A timestamp taken the first time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate). Used for calculating TELAPSE.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>A timestamp taken the last time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate).</dd>
 * <dt>Flip_X</dt> <dd>A boolean, if TRUE flip the image data in the X (horizontal) direction.</dd>
 * <dt>Flip_Y</dt> <dd>A boolean, if TRUE flip the image data in the Y (vertical) direction.</dd>
 * </dl>
 */
struct Bias_Dark_Struct
{
	double CCD_Temperature;
	char CCD_Temperature_Status_String[64];
	double Requested_Exposure_Length;
	int Image_Index;
	int Image_Count;
	struct timespec Multrun_Start_Time;
	struct timespec Exposure_Start_Time;
	int Flip_X;
	int Flip_Y;
};
	
/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Data holding local data to moptop bias and darks.
 * <dl>
 * <dt>CCD_Temperature</dt>               <dd>0.0</dd>
 * <dt>CCD_Temperature_Status_String</dt> <dd>""</dd>
 * <dt>Requested_Exposure_Length</dt>     <dd>0.0</dd>
 * <dt>Image_Index</dt>                   <dd>0</dd>
 * <dt>Image_Count</dt>                   <dd>0</dd>
 * <dt>Multrun_Start_Time</dt>            <dd>{0,0}</dd>
 * <dt>Exposure_Start_Time</dt>           <dd>{0,0}</dd>
 * <dt>Flip_X</dt>                        <dd>FALSE</dd>
 * <dt>Flip_Y</dt>                        <dd>FALSE</dd>
 * </dl>
 * @see #Bias_Dark_Struct
 */
static struct Bias_Dark_Struct Bias_Dark_Data =
{
	0.0,"",0.0,-1,0,{0,0},{0,0},FALSE,FALSE
};

/**
 * Is a bias or dark in progress.
 */
static int Bias_Dark_In_Progress = FALSE;
/**
 * Abort any bias or dark currently in progress.
 */
static int Bias_Dark_Abort = FALSE;

/* internal functions */
static int Bias_Dark_Setup(void);
static int Bias_Dark_Acquire_Images(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,
				    char ***filename_list,int *filename_count);
static int Bias_Dark_Get_Fits_Filename(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,
				       char *filename,int filename_length);
static int Bias_Dark_Write_Fits_Image(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,int andor_exposure_length_ms,
				      struct timespec exposure_end_time,long long int camera_ticks,
				      unsigned char *image_buffer,
				      int image_buffer_length,char *filename);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to configure how the image is orientated on readout.
 * @param flip_x A boolean, if TRUE the readout is flipped in the X (horizontal) direction after readout.
 * @param flip_y A boolean, if TRUE the readout is flipped in the Y (vertical) direction after readout.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Multrun_Data
 * @see #Moptop_General_Error_Number
 * @see #Moptop_General_Error_String
 */
int Moptop_Bias_Dark_Flip_Set(int flip_x,int flip_y)
{
	if(!MOPTOP_GENERAL_IS_BOOLEAN(flip_x))
	{
		Moptop_General_Error_Number = 700;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_Flip_Set: flip_x (%d) not a boolean.",flip_x);
		return FALSE;
	}
	if(!MOPTOP_GENERAL_IS_BOOLEAN(flip_y))
	{
		Moptop_General_Error_Number = 701;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_Flip_Set: flip_y (%d) not a boolean.",flip_y);
		return FALSE;
	}
	Bias_Dark_Data.Flip_X = flip_x;
	Bias_Dark_Data.Flip_Y = flip_y;
	return TRUE;
}

/**
 * Take a number of bias frames.
 * <ul>
 * <li>We initialise the filename_list and filename_count.
 * <li>We initialise Moptop_Abort to FALSE, and Moptop_In_Progress to TRUE.
 * <li>We check the exposure count is sensible, and initialise BiasDark_Data.Image_Count to it.
 * <li>We call Bias_Dark_Setup to do some common setup tasks.
 * <li>We call CCD_Command_Get_Exposure_Time_Min to get the minimum exposure length the camera will allow. We
 *     use this as the exposure length for the bias frames.
 * <li>We queue the image buffers for the acquisitions using CCD_Buffer_Queue_Images.
 * <li>We set the exposure length to the minimum exposure length the camera will allow using CCD_Exposure_Length_Set.
 * <li>We reset the cameras internal timestamp clock using CCD_Command_Timestamp_Clock_Reset.
 * <li>We set the camera shutter to stay closed using CCD_Command_Set_Shutter_Mode.
 * <li>We set the camera to trigger internally using CCD_Command_Set_Trigger_Mode.
 * <li>We set the camera to continuously take images using CCD_Command_Set_Cycle_Mode to set the cycle mode
 *     to CCD_COMMAND_CYCLE_MODE_CONTINUOUS.
 * <li>We set the number of frames to take using CCD_Command_Set_Frame_Count.
 * <li>We enable the camera for acquisitions using CCD_Command_Acquisition_Start.
 * <li>We call Bias_Dark_Acquire_Images to collect the taken images and save them to disk.
 * <li>We disable the camera for acquisition using CCD_Command_Acquisition_Stop.
 * <li>We set the camera to trigger from software using CCD_Command_Set_Trigger_Mode.
 * <li>We flush the camera (and associated queued image buffers) using CCD_Command_Flush.
 * <li>We set Moptop_In_Progress to FALSE.
 * </ul>
 * @param exposure_count The number of bias frames to take.
 * @param filename_list The address of a list of filenames of FITS images acquired during this set of biases.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the biases succeeds, returns FALSE if an error occurs or the biases are aborted.
 * @see #Bias_Dark_Data
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_In_Progress
 * @see #Bias_Dark_Setup
 * @see #Bias_Dark_Acquire_Images
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Queue_Images
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_CYCLE_MODE_FIXED
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_SHUTTER_MODE_CLOSED
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_INTERNAL
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Start
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Stop
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Flush
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Exposure_Time_Min
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Timestamp_Clock_Reset
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Shutter_Mode
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Trigger_Mode
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Cycle_Mode
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Frame_Count
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Set
 */
int Moptop_Bias_Dark_MultBias(int exposure_count,char ***filename_list,int *filename_count)
{
	double minimum_exposure_length_s;
	int bias_exposure_length_ms;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_TERSE,"BIAS",
				  "(exposure_count = %d,filename_list = %p,filename_count = %p) started.",
				  exposure_count,filename_list,filename_count);
#endif
	/* check arguments */
	if(filename_list == NULL)
	{
		Moptop_General_Error_Number = 702;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias: filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)
	{
		Moptop_General_Error_Number = 703;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias: filename_count was NULL.");
		return FALSE;
	}
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* initialise abort and in progress flags */
	Bias_Dark_Abort = FALSE;
	Bias_Dark_In_Progress = TRUE;
	/* how many images are we acquiring? */
	if(exposure_count < 1)
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 704;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias: exposure_count was too small (%d).",
			exposure_count);
		return FALSE;
	}
	Bias_Dark_Data.Image_Count = exposure_count;
	/* Do some common setup tasks for the bias/dark */
	if(!Bias_Dark_Setup())
	{
		Bias_Dark_In_Progress = FALSE;
		/* error message already set by Bias_Dark_Setup. */
		return FALSE;
	}
	/* get the minimum exposure length for this camera, in decimal seconds */
	if(!CCD_Command_Get_Exposure_Time_Min(&minimum_exposure_length_s))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 753;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to get minimum exposure length.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_INTERMEDIATE,
				  "BIAS","Minumum camera exposure length is %.3f seconds.",minimum_exposure_length_s);
#endif
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 705;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to queue image buffers.");
		return FALSE;
	}
	/* set exposure length to smallest allowed, in milliseconds */
	bias_exposure_length_ms = (int)(minimum_exposure_length_s*((double)MOPTOP_GENERAL_ONE_SECOND_MS));
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_INTERMEDIATE,
				  "BIAS","Attempting to set bias exposure length to %d ms.",bias_exposure_length_ms);
#endif
	if(!CCD_Exposure_Length_Set(bias_exposure_length_ms))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 719;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_MultBias: Failed to set bias exposure length to %d ms.",
			bias_exposure_length_ms);
		return FALSE;
	}
	Bias_Dark_Data.Requested_Exposure_Length = (((double)bias_exposure_length_ms)/
						    ((double)MOPTOP_GENERAL_ONE_SECOND_MS));
	/* reset internal clock timestamp */
	if(!CCD_Command_Timestamp_Clock_Reset())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 706;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to reset camera timestamp clock.");
		return FALSE;
	}
	/* set shutter mode to close */
	if(!CCD_Command_Set_Shutter_Mode(CCD_COMMAND_SHUTTER_MODE_CLOSED))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 751;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:"
			"Failed to set camera shutter mode to closed.");
		return FALSE;
	}
	/* turn on camera internal triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 707;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:"
			"Failed to set camera trigger mode to internal.");
		return FALSE;
	}
	/* bias and darks need cycle mode to be fixed */
	if(!CCD_Command_Set_Cycle_Mode(CCD_COMMAND_CYCLE_MODE_FIXED))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 720;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:CCD_Command_Set_Cycle_Mode(%s) failed.",
			CCD_COMMAND_CYCLE_MODE_FIXED);
		return FALSE;
	}
	/* set frame count to number of images to be acquired */
	if(!CCD_Command_Set_Frame_Count(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 721;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:CCD_Command_Set_Frame_Count(%d) failed.",
			Bias_Dark_Data.Image_Count);
		return FALSE;
	}				     

	/* enable the CCD acquisition */
	if(!CCD_Command_Acquisition_Start())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 708;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to start camera acquisition.");
		return FALSE;
	}
	/* acquire images */
	if(!Bias_Dark_Acquire_Images(CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS,filename_list,filename_count))
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		CCD_Command_Acquisition_Stop();
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		return FALSE;
	}
	/* stop CCD acquisition */
	if(!CCD_Command_Acquisition_Stop())
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		Moptop_General_Error_Number = 722;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to stop camera acquisition.");
		return FALSE;
	}
	/* check camera triggering is still software */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE))
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		Moptop_General_Error_Number = 723;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to set camera trigger mode to software.");
		return FALSE;
	}		
	if(!CCD_Command_Flush())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 724;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to flush camera.");
		return FALSE;
	}	
	Bias_Dark_In_Progress = FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_TERSE,"BIAS",
			   "finished.");
#endif
	return TRUE;
}

/**
 * Take a number of dark frames.
 * <ul>
 * <li>We initialise the filename_list and filename_count.
 * <li>We initialise Moptop_Abort to FALSE, and Moptop_In_Progress to TRUE.
 * <li>We check the exposure count is sensible, and initialise BiasDark_Data.Image_Count to it.
 * <li>We call Bias_Dark_Setup to do some common setup tasks.
 * <li>We queue the image buffers for the acquisitions using CCD_Buffer_Queue_Images.
 * <li>We check the requested exposure length is legal by calling CCD_Command_Get_Exposure_Time_Min and
 *     CCD_Command_Get_Exposure_Time_Max to get the allowed exposure lengths from the camera, and then
 *     comparing exposure_length_ms to them (after converting to ms).
 * <li>We set the exposure length to use using CCD_Exposure_Length_Set.
 * <li>We reset the cameras internal timestamp clock using CCD_Command_Timestamp_Clock_Reset.
 * <li>We set the camera to trigger internally (from the rotator) using CCD_Command_Set_Trigger_Mode.
 * <li>We set the camera to continuously take images using CCD_Command_Set_Cycle_Mode to set the cycle mode
 *     to CCD_COMMAND_CYCLE_MODE_CONTINUOUS.
 * <li>We set the number of frames to take using CCD_Command_Set_Frame_Count.
 * <li>We enable the camera for acquisitions using CCD_Command_Acquisition_Start.
 * <li>We call Bias_Dark_Acquire_Images to collect the taken images and save them to disk.
 * <li>We disable the camera for acquisition using CCD_Command_Acquisition_Stop.
 * <li>We set the camera to trigger from software using CCD_Command_Set_Trigger_Mode.
 * <li>We flush the camera (and associated queued image buffers) using CCD_Command_Flush.
 * <li>We set Moptop_In_Progress to FALSE.
 * </ul>
 * @param exposure_count The number of dark frames to take.
 * @param filename_list The address of a list of filenames of FITS images acquired during this set of dark frames.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the dark frames succeeds, returns FALSE if an error occurs or the dark frames are aborted.
 * @see #Bias_Dark_Data
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_In_Progress
 * @see #Bias_Dark_Setup
 * @see #Bias_Dark_Acquire_Images
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 * @see ../ccd/cdocs/ccd_buffer.html#CCD_Buffer_Queue_Images
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_CYCLE_MODE_FIXED
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_INTERNAL
 * @see ../ccd/cdocs/ccd_command.html#CCD_COMMAND_TRIGGER_MODE_SOFTWARE
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Start
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Stop
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Flush
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Exposure_Time_Min
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Exposure_Time_Max
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Timestamp_Clock_Reset
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Trigger_Mode
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Cycle_Mode
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Set_Frame_Count
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Set
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK
 */
int Moptop_Bias_Dark_MultDark(int exposure_length_ms,int exposure_count,
				     char ***filename_list,int *filename_count)
{
	double minimum_exposure_length_s,maximum_exposure_length_s;
	int minimum_exposure_length_ms, maximum_exposure_length_ms;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("dark","moptop_bias_dark.c","Moptop_Bias_Dark_MultDark",LOG_VERBOSITY_TERSE,"DARK",
				  "(exposure_length_ms = %d,exposure_count = %d,"
				  "filename_list = %p,filename_count = %p) started.",
				  exposure_length_ms,exposure_count,filename_list,filename_count);
#endif
	/* check arguments */
	if(filename_list == NULL)
	{
		Moptop_General_Error_Number = 709;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark: filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)
	{
		Moptop_General_Error_Number = 710;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark: filename_count was NULL.");
		return FALSE;
	}
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* initialise abort and in progress flags */
	Bias_Dark_Abort = FALSE;
	Bias_Dark_In_Progress = TRUE;
	/* how many images are we acquiring? */
	if(exposure_count < 1)
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 711;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark: exposure_count was too small (%d).",
			exposure_count);
		return FALSE;
	}
	Bias_Dark_Data.Image_Count = exposure_count;
	/* Do some common setup tasks for the bias/dark */
	if(!Bias_Dark_Setup())
	{
		Bias_Dark_In_Progress = FALSE;
		/* error message already set by Bias_Dark_Setup. */
		return FALSE;
	}
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 712;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to queue image buffers.");
		return FALSE;
	}
	/* check exposure length is legal */
	/* get minimum exposure length the camera allows, in seconds */
	if(!CCD_Command_Get_Exposure_Time_Min(&minimum_exposure_length_s))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 754;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to get minimum exposure length.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_INTERMEDIATE,
				  "BIAS","Minumum camera exposure length is %.3f seconds.",minimum_exposure_length_s);
#endif
	/* get maximum exposure length the camera allows, in seconds */
	if(!CCD_Command_Get_Exposure_Time_Max(&maximum_exposure_length_s))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 755;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to get maximum exposure length.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias","moptop_bias_dark.c","Moptop_Bias_Dark_MultBias",LOG_VERBOSITY_INTERMEDIATE,
				  "BIAS","Maxumum camera exposure length is %.3f seconds.",maximum_exposure_length_s);
#endif
	/* convert minimum/maximum exposure lengths to milliseconds */
	minimum_exposure_length_ms = (int)(minimum_exposure_length_s*((double)MOPTOP_GENERAL_ONE_SECOND_MS));
	maximum_exposure_length_ms = (int)(maximum_exposure_length_s*((double)MOPTOP_GENERAL_ONE_SECOND_MS));
	if((exposure_length_ms < minimum_exposure_length_ms)||(exposure_length_ms > maximum_exposure_length_ms))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 756;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_MultDark: Requested exposure length %d ms out of allowed range (%d,%d).",
			exposure_length_ms,minimum_exposure_length_ms,maximum_exposure_length_ms);
		return FALSE;
	}
	/* set exposure length  */
	if(!CCD_Exposure_Length_Set(exposure_length_ms))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 713;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_MultDark: Failed to set exposure length to %d ms.",exposure_length_ms);
		return FALSE;
	}
	Bias_Dark_Data.Requested_Exposure_Length = (((double)exposure_length_ms)/
						    ((double)MOPTOP_GENERAL_ONE_SECOND_MS));
	/* reset internal clock timestamp */
	if(!CCD_Command_Timestamp_Clock_Reset())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 714;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:"
			"Failed to reset camera timestamp clock.");
		return FALSE;
	}
	/* turn on camera internal triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_INTERNAL))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 715;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:"
			"Failed to set camera trigger mode to internal.");
		return FALSE;
	}
	/* bias and darks need cycle mode to be fixed */
	if(!CCD_Command_Set_Cycle_Mode(CCD_COMMAND_CYCLE_MODE_FIXED))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 725;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:CCD_Command_Set_Cycle_Mode(%s) failed.",
			CCD_COMMAND_CYCLE_MODE_FIXED);
		return FALSE;
	}
	/* set frame count to number of images to be acquired */
	if(!CCD_Command_Set_Frame_Count(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 726;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:CCD_Command_Set_Frame_Count(%d) failed.",
			Bias_Dark_Data.Image_Count);
		return FALSE;
	}				     
	/* enable the CCD acquisition */
	if(!CCD_Command_Acquisition_Start())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 716;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to start camera acquisition.");
		return FALSE;
	}
	/* acquire images */
	if(!Bias_Dark_Acquire_Images(CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK,filename_list,filename_count))
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		CCD_Command_Acquisition_Stop();
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		return FALSE;
	}
	/* stop CCD acquisition */
	if(!CCD_Command_Acquisition_Stop())
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE);
		Moptop_General_Error_Number = 727;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to stop camera acquisition.");
		return FALSE;
	}
	/* check camera triggering is still software */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE))
	{
		Bias_Dark_In_Progress = FALSE;
		CCD_Command_Flush();
		Moptop_General_Error_Number = 728;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to set camera trigger mode to software.");
		return FALSE;
	}		
	if(!CCD_Command_Flush())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 729;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultDark:Failed to flush camera.");
		return FALSE;
	}
	Bias_Dark_In_Progress = FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("dark","moptop_bias_dark.c","Moptop_Bias_Dark_MultDark",LOG_VERBOSITY_TERSE,"DARK","finished.");
#endif
	return TRUE;
}

/**
 * Abort a currently running bias or dark.
 * <ul>
 * <li>Sets Bias_Dark_Abort to TRUE.
 * <li>If Bias_Dark_In_Progress is true:
 *     <ul>
 *     <li>Stop camera acquisition using CCD_Command_Acquisition_Stop.
 *     </ul>
 * <li>Returns Bias_Dark_In_Progress (i.e. whether there was a bias or dark in progress to be aborted).
 * </ul>
 * @return The routine returns TRUE if the bias or dark was aborted, FALSE otherwise.
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_In_Progress
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Acquisition_Stop
 */
int Moptop_Bias_Dark_Abort(void)
{
	Bias_Dark_Abort = TRUE;
	if(Bias_Dark_In_Progress)
	{
		/* stop the camera acquisition */
		if(!CCD_Command_Acquisition_Stop())
		{
			Moptop_General_Error_Number = 730;
			sprintf(Moptop_General_Error_String,
				"Moptop_Bias_Dark_Abort:Failed to stop camera acquisition.");
			return FALSE;
		}
	}/* end if Bias_Dark_In_Progress */
	/* allow aborted bias/dark to call CCD_Command_Flush rather than call it here */
	return TRUE;
}

/**
 * Return whether a bias or dark is in progress.
 * @return TRUE if a bias or dark is in progress, FALSE otherwise.
 * @see #Bias_Dark_In_Progress
 */
int Moptop_Bias_Dark_In_Progress(void)
{
	return Bias_Dark_In_Progress;
}

/**
 * Return the total number of exposures expected to be generated in the current/last bias/dark.
 * @return The number of images/frames expected.
 * @see #Bias_Dark_Data
 */
int Moptop_Bias_Dark_Count_Get(void)
{
	return Bias_Dark_Data.Image_Count;
}

/**
 * Return the exposure length of an individual frame in the bias/dark.
 * @return The exposure length of an individual frame in milliseconds.
 * @see #Bias_Dark_Data
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 */
int Moptop_Bias_Dark_Per_Frame_Exposure_Length_Get(void)
{
	return (int)(Bias_Dark_Data.Requested_Exposure_Length*((double)MOPTOP_GENERAL_ONE_SECOND_MS));
}

/**
 * Return the exposure start time timestamp of the last exposure in the bias/dark multrun.
 * @param exposure_start_time The address of a timespec structure to fill with the start time timestamp.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Bias_Dark_Data
 */
int Moptop_Bias_Dark_Exposure_Start_Time_Get(struct timespec *exposure_start_time)
{
	if(exposure_start_time == NULL)
	{
		Moptop_General_Error_Number = 752;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_Exposure_Start_Time_Get:exposure_start_time was NULL.");
		return FALSE;
	}
	(*exposure_start_time) = Bias_Dark_Data.Exposure_Start_Time;
	return TRUE;
}

/**
 * Return which exposure in the bias/dark multrun we are on.
 * @return The exposure index in the bias/dark multrun.
 * @see #Multrun_Data
 */
int Moptop_Bias_Dark_Exposure_Index_Get(void)
{
	return Bias_Dark_Data.Image_Index;
}

/**
 * Return the multrun number (in the generated FITS filenames) of this bias/dark.
 * @return The current bias/dark number.
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 */
int Moptop_Bias_Dark_Multrun_Get(void)
{
	return CCD_Fits_Filename_Multrun_Get();
}

/**
 * Return the run number (in the generated FITS filenames) of this bias/dark.
 * @return The current run number.
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Run_Get
 */
int Moptop_Bias_Dark_Run_Get(void)
{
	return CCD_Fits_Filename_Run_Get();
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Setup routine for doing bias/darks.
 * <ul>
 * <li>Increment the multrun number (CCD_Fits_Filename_Next_Multrun).
 * <li>Set the current CCD temperature and CCD temperature status and store them in Bias_Dark_Data 
 *     (for later inclusion in the FITS headers).
 * <li>Set the image data flipping (Moptop_Bias_Dark_Flip_Set) from the relevant config 
 *     ("moptop.multrun.image.flip.x|y").
 * </ul>
 * @return The routine returns TRUE on success, and FALSE if an error occurs.
 * @see #Bias_Dark_Data
 * @see #Moptop_Bias_Dark_Flip_Set
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Multrun
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 */
static int Bias_Dark_Setup(void)
{
	int flip_x,flip_y;
	
	/* increment the multrun number */
	CCD_Fits_Filename_Next_Multrun();
	/* get current CCD temperature/status and store it for later */
	if(!CCD_Temperature_Get(&(Bias_Dark_Data.CCD_Temperature)))
	{
		Moptop_General_Error_Number = 717;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Setup: Failed to get CCD temperature.");
		return FALSE;		
	}
	if(!CCD_Temperature_Get_Temperature_Status_String(Bias_Dark_Data.CCD_Temperature_Status_String,64))
	{
		Moptop_General_Error_Number = 718;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Setup: Failed to get CCD temperature status string.");
		return FALSE;		
	}
	/* configure flipping of output image. Use same config as multruns, so bias and darks match. */
	if(!Moptop_Config_Get_Boolean("moptop.multrun.image.flip.x",&flip_x))
		return FALSE;		
	if(!Moptop_Config_Get_Boolean("moptop.multrun.image.flip.y",&flip_y))
		return FALSE;		
	Moptop_Bias_Dark_Flip_Set(flip_x,flip_y);
	return TRUE;
}


/**
 * Routine to actually acquire the bias/dark images.
 * <ul>
 * <li>We check the filename_list and filename_count are not NULL and initialise them.
 * <li>We get the camera exposure length using CCD_Exposure_Length_Get.
 * <li>We get the camera's internal clock frequency using CCD_Command_Get_Timestamp_Clock_Frequency.
 * <li>We calculate a timeout as being twice the exposure length plus one second.
 * <li>We initialise last_camera_ticks to zero.
 * <li>We loop over the Bias_Dark_Data.Image_Count, using Bias_Dark_Data.Image_Index as an index counter 
 *     (for status reporting):
 *     <ul>
 *     <li>We take a timestamp and store it in Bias_Dark_Data.Exposure_Start_Time.
 *     <li>If this is the first exposure in the multrun we set Bias_Dark_Data.Multrun_Start_Time to the same timestamp.
 *     <li>We wait for a readout by calling CCD_Command_Wait_Buffer.
 *     <li>We get the camera image timestamp from the image metadata using CCD_Command_Get_Timestamp_From_Metadata.
 *     <li>We calculate the camera_clock_difference as the difference in camera ticks between this image and the 
 *         last one, divided by the previously retrieved camera's internal clock frequency.
 *     <li>We get an exposure end timestamp and store it in exposure_end_time.
 *     <li>We call Bias_Dark_Get_Fits_Filename to generate a new FITS filename.
 *     <li>We call Bias_Dark_Write_Fits_Image to write the image data to the generated FTYS filename.
 *     <li>We add the generated filename to the filename list using CCD_Fits_Filename_List_Add.
 *     <li>We check whether the bias/dark has been aborted (Bias_Dark_Abort).
 *     </ul>
 * <li>
 * </ul>
 * @param exposure_type A CCD_FITS_FILENAME_EXPOSURE_TYPE enum, one of:
 *        CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS or CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK.
 * @param filename_list The address of a list of filenames of FITS images acquired during this multrun.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #BIAS_DARK_FITS_FILENAME_LENGTH
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_Get_Fits_Filename
 * @see #Bias_Dark_Write_Fits_Image
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Wait_Buffer
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Timestamp_Clock_Frequency
 * @see ../ccd/cdocs/ccd_command.html#CCD_Command_Get_Timestamp_From_Metadata
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Length_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Add
 */
static int Bias_Dark_Acquire_Images(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,
				    char ***filename_list,int *filename_count)
{
	struct timespec exposure_end_time;
	char filename[BIAS_DARK_FITS_FILENAME_LENGTH];
	unsigned char *image_buffer = NULL;
	unsigned int timeout_ms;
	double camera_clock_difference;
	long long int camera_ticks,last_camera_ticks,timestamp_clock_frequency;
	int andor_exposure_length_ms;
	int image_buffer_length;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("bias_dark","moptop_bias_dark.c","Bias_Dark_Acquire_Images",
				  LOG_VERBOSITY_INTERMEDIATE,"BIASDARK",
				  "started with image count %d, exposure_type = %d.",Bias_Dark_Data.Image_Count,
				  exposure_type);
#endif
	/* check arguments */
	if(filename_list == NULL)
	{
		Moptop_General_Error_Number = 731;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images: filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)
	{
		Moptop_General_Error_Number = 732;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images: filename_count was NULL.");
		return FALSE;
	}
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* compute how long CCD_Command_Wait_Buffer should wait for an image to arrive, in milliseconds */
	/* get exposure length used by andor */
	if(!CCD_Exposure_Length_Get(&andor_exposure_length_ms))
	{
		Moptop_General_Error_Number = 733;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:Failed to get Andor exposure length.");
		return FALSE;
	}
	/* get camera's internal clock frequency for use in the loop */
	if(!CCD_Command_Get_Timestamp_Clock_Frequency(&timestamp_clock_frequency))
	{
		Moptop_General_Error_Number = 734;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:"
			"Failed to get timestamp clock frequency.");
		return FALSE;
	}
	/* calculate a timeout. Note for a bias andor_exposure_length_ms can be near/equal to 0. */
	timeout_ms = (andor_exposure_length_ms+1000) * 2;
	last_camera_ticks = 0;
	/* acquire frames */
	for(Bias_Dark_Data.Image_Index=0;Bias_Dark_Data.Image_Index < Bias_Dark_Data.Image_Count;
	    Bias_Dark_Data.Image_Index++)
	{
		/* get exposure start timestamp */
		clock_gettime(CLOCK_REALTIME,&(Bias_Dark_Data.Exposure_Start_Time));
		/* If this is the first exposure in the multrun, 
		** the exposure start time is also the multrun start time. */
		if(Bias_Dark_Data.Image_Index == 0)
			Bias_Dark_Data.Multrun_Start_Time = Bias_Dark_Data.Exposure_Start_Time;
		/* get an acquired image buffer */
		if(!CCD_Command_Wait_Buffer(&image_buffer,&image_buffer_length,timeout_ms))
		{
			Moptop_General_Error_Number = 735;
			sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:"
				"Failed to retrieve image buffer.");
			return FALSE;
		}
		/* get camera timestamp */
		if(!CCD_Command_Get_Timestamp_From_Metadata(image_buffer,image_buffer_length,&camera_ticks))
		{
			Moptop_General_Error_Number = 736;
			sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:"
				"Failed to get timestamp from metadata.");
			return FALSE;
		}
		camera_clock_difference = ((double)(camera_ticks-last_camera_ticks))/(double)timestamp_clock_frequency;
		last_camera_ticks = camera_ticks;
		/* get exposure end timestamp */
		clock_gettime(CLOCK_REALTIME,&exposure_end_time);
		/* generate a new filename for this FITS image */
		Bias_Dark_Get_Fits_Filename(exposure_type,filename,BIAS_DARK_FITS_FILENAME_LENGTH);
		/* write fits image */
		if(!Bias_Dark_Write_Fits_Image(exposure_type,andor_exposure_length_ms,exposure_end_time,camera_ticks,
					       image_buffer,image_buffer_length,filename))
		{
			return FALSE;
		}
		/* add fits image to list */
		if(!CCD_Fits_Filename_List_Add(filename,filename_list,filename_count))
		{
			Moptop_General_Error_Number = 737;
			sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:"
				"Failed to add filename '%s' to list of filenames (count = %d).",
				filename,(*filename_count));
			return FALSE;
		}
		/* check for abort */
		if(Bias_Dark_Abort)
		{
			Moptop_General_Error_Number = 738;
			sprintf(Moptop_General_Error_String,"Bias_Dark_Acquire_Images:Bias/Dark Aborted.");
			return FALSE;
		}
	}/* end for on Bias_Dark_Data.Image_Index / Bias_Dark_Data.Image_Count */
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("biasdark","moptop_bias_dark.c","Bias_Dark_Acquire_Images",LOG_VERBOSITY_INTERMEDIATE,
			   "BIASDARK","finished.");
#endif
	return TRUE;
}

/**
 * Generate the next FITS filename to write image data into.
 * <ul>
 * <li>Increment the run number by calling CCD_Fits_Filename_Next_Run.
 * <li>Generate an unreduced FITS filename by calling CCD_Fits_Filename_Get_Filename.
 * </ul>
 * @param exposure_type An CCD_FITS_FILENAME_EXPOSURE_TYPE, one of:
 *        CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS or CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK.
 * @param filename A previously allocated string to write the generated FITS image filename into.
 * @param filename_length The length of the filename buffer to store a filename in characters.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Run
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Get_Filename
 */
static int Bias_Dark_Get_Fits_Filename(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,
				       char *filename,int filename_length)
{
	/* increment the run number */
	CCD_Fits_Filename_Next_Run();
	if(!CCD_Fits_Filename_Get_Filename(exposure_type,CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED,
					   filename,filename_length))
	{
		Moptop_General_Error_Number = 739;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Get_Fits_Filename:Getting filename failed.");
		return FALSE;		
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log_Format("biasdark","moptop_bias_dark.c","Bias_Dark_Get_Fits_Filename",
				  LOG_VERBOSITY_INTERMEDIATE,"BIASDARK","New filename generated: '%s'.",filename);
#endif
	return TRUE;
}
/* diddly delete
 * <li>We set the "FILTER1" FITS keyword value based on the cached filter name in Multrun_Data.Filter_Name.
 * <li>We set the "FILTERI1" FITS keyword value based on the cached filter name in Multrun_Data.Filter_Id.
 * <li>We set the "MOPRMODE" FITS keyword value to the requested rotator speed stored in Multrun_Data.Rotator_Speed.
 * <li>We set the "MOPRRATE" FITS keyword value to the rotator angular velocity stored in Multrun_Data.Rotator_Run_Velocity.
 * <li>We set the "MOPRREQ" FITS keyword value to the requested_rotator_angle.
 * <li>We set the "MOPRBEG" FITS keyword value to the rotator_start_angle.
 * <li>We set the "MOPREND" FITS keyword value to the rotator_end_angle.
 * <li>We set the "MOPRARC" FITS keyword value to the rotator_difference.
 * <li>We set the "MOPRNUM" FITS keyword value to the Multrun_Data.Rotation_Number.
 * <li>We set the "MOPRPOS" FITS keyword value to the Multrun_Data.Sequence_Number.
 */
/**
 * Write the FITS image to disk.
 * <ul>
 * <li>We set the "OBSTYPE" FITS keyword value based on the value of exposure_type.
 * <li>We set the "DATE"/"DATE-OBS"/"UTSTART" and "MJD" keyword values based on the value of Multrun_Data.Exposure_Start_Time.
 * <li>We set the "DATE-END" and "UTEND" keyword values based on the value of exposure_end_time.
 * <li>We set the "TELAPSE" keyword value based on the time elapsed between 
 *     Multrun_Data.Multrun_Start_Time and exposure_end_time.
 * <li>We set the "RUNNUM" keyword value to Multrun_Data.Image_Index.
 * <li>We set the "EXPNUM" keyword value to Multrun_Data.Image_Index.
 * <li>We set the "EXPTOTAL" keyword value to Multrun_Data.Image_Count.
 * <li>We set the "CCDXBIN"/"CCDYBIN" FITS keyword values based on CCD_Setup_Get_Binning. 
 * <li>We set the "CCDATEMP" FITS keyword value based on the cached CCD temperature stored in 
 *     Multrun_Data.CCD_Temperature.
 * <li>We set the "TEMPSTAT" FITS keyword value based on the cached CCD temperature status stored in
 *     Multrun_Data.CCD_Temperature_Status_String.
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
 * <li>If Multrun_Data.Flip_X is TRUE, we call Moptop_Multrun_Flip_X to flip the image data in the X direction.
 * <li>If Multrun_Data.Flip_Y is TRUE, we call Moptop_Multrun_Flip_Y to flip the image data in the Y direction.
 * <li>We write the image data to the FITS image using fits_write_img.
 * <li>If the binning value is not 1, we retrieve the current CCDSCALE value, scale it by the binning, 
 *     and update the FITS keyword value.
 * <li>We close the FITS image using fits_close_file.
 * <li>We remove the file lock on the FITS image using CCD_Fits_Filename_UnLock.
 * </ul>
 * @param exposure_type An CCD_FITS_FILENAME_EXPOSURE_TYPE, one of:
 *        CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS or CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK.
 * @param andor_exposure_length_ms The exposure length as retrieved from the camera, in milliseconds.
 * @param exposure_end_time A timestamp representing the end time of the exposure being saved. Actually a timestamp
 *        taken just after the camera has signalled a buffer is available.
 * @param camera_ticks A long long int holding the camera clock ticks stored by the camera in it's meta-data at the
 *        instant it began this exposure.
 * @param image_buffer The image buffer containing the data to write to disk.
 * @param image_buffer_length The length of data in the image buffer (note this includes metadata).
 * @param filename A string containing the FITS filename to write the data into.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Bias_Dark_Data
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
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_SECOND_MS
 * @see moptop_general.html#MOPTOP_GENERAL_ONE_METRE_MICROMETRE
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Log_Format
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Lock
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_UnLock
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Width
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Sensor_Height
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Binning
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Width
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Pixel_Height
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Get_Timestamp_Clock_Frequency
 */
static int Bias_Dark_Write_Fits_Image(enum CCD_FITS_FILENAME_EXPOSURE_TYPE exposure_type,int andor_exposure_length_ms,
				      struct timespec exposure_end_time,long long int camera_ticks,
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
	Moptop_General_Log_Format("biasdark","moptop_bias_dark.c","Bias_Dark_Write_Fits_Image",
				  LOG_VERBOSITY_INTERMEDIATE,
				  "BIASDARK","Started saving FITS filename '%s'.",filename);
#endif
	/* sort out some FITS headers for later */
	/* OBSTYPE */
	if(exposure_type == CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS)
	{
		if(!Moptop_Fits_Header_String_Add("OBSTYPE","BIAS",NULL))
			return FALSE;
	}
	else if(exposure_type == CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK)
	{
		if(!Moptop_Fits_Header_String_Add("OBSTYPE","DARK",NULL))
			return FALSE;
	}
	else
	{
		Moptop_General_Error_Number = 740;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:Illegal exposure type %d.",
			exposure_type);
		return FALSE;
	}
	/* FILTER1 */
	/* diddly delete
	if(!Moptop_Fits_Header_String_Add("FILTER1",Bias_Dark_Data.Filter_Name,NULL))
		return FALSE;
	*/
	/* FILTERI1 */
	/* diddly delete
	if(!Moptop_Fits_Header_String_Add("FILTERI1",Bias_Dark_Data.Filter_Id,NULL))
		return FALSE;
	*/
	/* update DATE keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_Date_String(Bias_Dark_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("DATE",exposure_time_string,"[UTC] Start date of obs."))
		return FALSE;
	/* update DATE-OBS keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_Date_Obs_String(Bias_Dark_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("DATE-OBS",exposure_time_string,"[UTC] Start of obs."))
		return FALSE;
	/* update UTSTART keyword from Multrun_Data.Exposure_Start_Time */
	Moptop_Fits_Header_TimeSpec_To_UtStart_String(Bias_Dark_Data.Exposure_Start_Time,exposure_time_string);
	if(!Moptop_Fits_Header_String_Add("UTSTART",exposure_time_string,"[UTC] Start time of obs."))
		return FALSE;
	/* update MJD keyword from Bias_Dark_Data.Exposure_Start_Time */
	/* note leap second correction not implemented yet (always FALSE). */
	if(!Moptop_Fits_Header_TimeSpec_To_Mjd(Bias_Dark_Data.Exposure_Start_Time,FALSE,&mjd))
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
	dvalue = fdifftime(exposure_end_time,Bias_Dark_Data.Multrun_Start_Time);
	if(!Moptop_Fits_Header_Float_Add("TELAPSE",dvalue,"[sec] Total obs. duration"))
		return FALSE;
	/* update RUNNUM keyword value with the Bias_Dark_Data.Image_Index */
	if(!Moptop_Fits_Header_Integer_Add("RUNNUM",Bias_Dark_Data.Image_Index,
					   "Which image in the multrun we are on."))
		return FALSE;
	/* update EXPNUM keyword value with the Bias_Dark_Data.Image_Index */
	if(!Moptop_Fits_Header_Integer_Add("EXPNUM",Bias_Dark_Data.Image_Index,
					   "Which image in the current multrun we are on."))
		return FALSE;
	/* update EXPTOTAL keyword value with Bias_Dark_Data.Image_Count */
	if(!Moptop_Fits_Header_Integer_Add("EXPTOTAL",Bias_Dark_Data.Image_Count,
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
	if(!Moptop_Fits_Header_Float_Add("CCDATEMP",Bias_Dark_Data.CCD_Temperature,NULL))
		return FALSE;
	/* update ccd temperature status string with value stored at start of multrun */
	if(!Moptop_Fits_Header_String_Add("TEMPSTAT",Bias_Dark_Data.CCD_Temperature_Status_String,NULL))
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
	/* EXPREQST is the requested exposure length in seconds (from Bias_Dark_Data.Requested_Exposure_Length) */
	if(!Moptop_Fits_Header_Float_Add("EXPREQST",Bias_Dark_Data.Requested_Exposure_Length,
					 "[sec] Requested exposure"))
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
	Moptop_General_Log_Format("biasdark","moptop_bias_dark.c","Bias_Dark_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,
				  "BIASDARK","Locking FITS filename %s.",filename);
#endif
	if(!CCD_Fits_Filename_Lock(filename))
	{
		Moptop_General_Error_Number = 741;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:Failed to lock '%s'.",filename);
		return FALSE;				
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log_Format("biasdark","moptop_bias_dark.c","Bias_Dark_Write_Fits_Image",
				  LOG_VERBOSITY_INTERMEDIATE,"BIASDARK","Saving to filename %s.",filename);
#endif
	/* create FITS file */
	retval = fits_create_file(&fp,filename,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 742;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:File create failed(%s,%d,%s).",
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
		Moptop_General_Error_Number = 743;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:create image failed(%s,%d,%s).",
			filename,status,buff);
		return FALSE;
	}
	/* save FITS headers to filename */
	if(!CCD_Fits_Header_Write_To_Fits(fp))
	{
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 744;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:CCD_Fits_Header_Write_To_Fits failed.");
		return FALSE;
	}
	/* save data to filename */
	if((ncols_binned*nrows_binned) > image_buffer_length)
	{
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 745;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:FITS image dimension mismatch:"
			"filename '%s', binned ncols = %d, binned_nrows = %d, image buffer length = %d.",
			filename,ncols_binned,nrows_binned,image_buffer_length);
		return FALSE;
	}
	/* check and flip images if configured to do so */
	if(Bias_Dark_Data.Flip_X)
		Moptop_Multrun_Flip_X(ncols_binned,nrows_binned,(unsigned short *)image_buffer);
	if(Bias_Dark_Data.Flip_Y)
		Moptop_Multrun_Flip_Y(ncols_binned,nrows_binned,(unsigned short *)image_buffer);
	/* write the data */
	retval = fits_write_img(fp,TUSHORT,1,ncols_binned*nrows_binned,image_buffer,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fp,&status);
		CCD_Fits_Filename_UnLock(filename);
		Moptop_General_Error_Number = 746;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:File write failed(%s,%d,%s).",
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
			Moptop_General_Error_Number = 747;
			sprintf(Moptop_General_Error_String,
				"Bias_Dark_Write_Fits_Image: Retrieving ccdscale failed(%s,%d,%s).",
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
			Moptop_General_Error_Number = 748;
			sprintf(Moptop_General_Error_String,
				"Bias_Dark_Write_Fits_Image: Updating ccdscale failed(%.2f,%s,%d,%s).",
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
		Moptop_General_Error_Number = 749;
		sprintf(Moptop_General_Error_String,
			"Bias_Dark_Write_Fits_Image: File close failed(%s,%d,%s).",filename,status,buff);
		return FALSE;
	}
	/* unlock FITS filename lock */
	if(!CCD_Fits_Filename_UnLock(filename))
	{
		Moptop_General_Error_Number = 750;
		sprintf(Moptop_General_Error_String,"Bias_Dark_Write_Fits_Image:Failed to unlock '%s'.",filename);
		return FALSE;
	}
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("biasdark","moptop_bias_dark.c","Bias_Dark_Write_Fits_Image",LOG_VERBOSITY_INTERMEDIATE,
			   "BIASDARK","Finished.");
#endif
	return TRUE;
}
