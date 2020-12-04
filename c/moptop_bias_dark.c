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
 * <li>We queue the image buffers for the acquisitions using CCD_Buffer_Queue_Images.
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * </ul>
 * @param exposure_count The number of bias frames to take.
 * @param filename_list The address of a list of filenames of FITS images acquired during this set of biases.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the biases succeeds, returns FALSE if an error occurs or the biases are aborted.
 * @see #Bias_Dark_Data
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_In_Progress
 */
int Moptop_Bias_Dark_MultBias(int exposure_count,char ***filename_list,int *filename_count)
{
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
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 705;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to queue image buffers.");
		return FALSE;
	}
	/* set exposure length to zero */
	if(!CCD_Exposure_Length_Set(0))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 706;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_MultBias: Failed to set exposure length to zero.");
		return FALSE;
	}
	Bias_Dark_Data.Requested_Exposure_Length = 0.0f;
	/* reset internal clock timestamp */
	if(!CCD_Command_Timestamp_Clock_Reset())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 706;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to reset camera timestamp clock.");
		return FALSE;
	}
	/* turn on camera internal triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 707;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to set camera trigger mode to software.");
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
	
	return TRUE;
}

/**
 * Take a number of dark frames.
 * <ul>
 * <li>We initialise the filename_list and filename_count.
 * <li>We initialise Moptop_Abort to FALSE, and Moptop_In_Progress to TRUE.
 * <li>We check the exposure count is sensible, and initialise BiasDark_Data.Image_Count to it.
 * <li>We queue the image buffers for the acquisitions using CCD_Buffer_Queue_Images.
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * <li>
 * </ul>
 * @param exposure_count The number of dark frames to take.
 * @param filename_list The address of a list of filenames of FITS images acquired during this set of dark frames.
 * @param filename_count The address of an integer to store the number of FITS images in filename_list.
 * @return Returns TRUE if the dark frames succeeds, returns FALSE if an error occurs or the dark frames are aborted.
 * @see #Bias_Dark_Data
 * @see #Bias_Dark_Abort
 * @see #Bias_Dark_In_Progress
 */
int Moptop_Bias_Dark_MultDark(int exposure_length_ms,int exposure_count,
				     char ***filename_list,int *filename_count)
{
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
	/* setup CCD_Buffer for image acquisition */
	if(!CCD_Buffer_Queue_Images(Bias_Dark_Data.Image_Count))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 712;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to queue image buffers.");
		return FALSE;
	}
	/* set exposure length  */
	if(!CCD_Exposure_Length_Set(exposure_length_ms))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 713;
		sprintf(Moptop_General_Error_String,
			"Moptop_Bias_Dark_MultBias: Failed to set exposure length to zero.");
		return FALSE;
	}
	Bias_Dark_Data.Requested_Exposure_Length = (((double)exposure_length_ms)/
						    ((double)MOPTOP_GENERAL_ONE_SECOND_MS));
	/* reset internal clock timestamp */
	if(!CCD_Command_Timestamp_Clock_Reset())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 714;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to reset camera timestamp clock.");
		return FALSE;
	}
	/* turn on camera internal triggering */
	if(!CCD_Command_Set_Trigger_Mode(CCD_COMMAND_TRIGGER_MODE_SOFTWARE))
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 715;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to set camera trigger mode to software.");
		return FALSE;
	}
	/* enable the CCD acquisition */
	if(!CCD_Command_Acquisition_Start())
	{
		Bias_Dark_In_Progress = FALSE;
		Moptop_General_Error_Number = 716;
		sprintf(Moptop_General_Error_String,"Moptop_Bias_Dark_MultBias:Failed to start camera acquisition.");
		return FALSE;
	}

	return TRUE;
}

int Moptop_Bias_Dark_Abort(void)
{
	return TRUE;
}

int Moptop_Bias_Dark_In_Progress(void)
{
	return Bias_Dark_In_Progress;
}

int Moptop_Bias_Dark_Count_Get(void)
{
	return 0;
}


int Moptop_Bias_Dark_Per_Frame_Exposure_Length_Get(void)
{
	return 0;
}

int Moptop_Bias_Dark_Multrun_Get(void)
{
	return 0;
}

int Moptop_Bias_Dark_Run_Get(void)
{
	return 0;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Setup routine for doing bias/darks.
 * <ul>
 * <li>Increment the multrun number (CCD_Fits_Filename_Next_Multrun).
 * <li>Increment the run number to one (CCD_Fits_Filename_Next_Run). 
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
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Next_Run
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 */
static int Bias_Dark_Setup(void)
{
	int flip_x,flip_y;
	
	/* increment the multrun number */
	CCD_Fits_Filename_Next_Multrun();
	/* increment the run number (effectively which rotation we are on) to one */
	CCD_Fits_Filename_Next_Run();
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
