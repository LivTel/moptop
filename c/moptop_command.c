/* moptop_command.c
** Moptop command routines
*/
/**
 * Command routines for Moptop.
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
#include <string.h>
/**
 * Add more fields to struct tm (tm_tm_zone).
 */
#define _BSD_SOURCE
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "ccd_exposure.h"
#include "ccd_fits_filename.h"
#include "ccd_fits_header.h"
#include "ccd_general.h"
#include "ccd_setup.h"
#include "ccd_temperature.h"

#include "command_server.h"

#include "filter_wheel_command.h"
#include "filter_wheel_config.h"
#include "filter_wheel_general.h"

#include "moptop_bias_dark.h"
#include "moptop_config.h"
#include "moptop_fits_header.h"
#include "moptop_multrun.h"
#include "moptop_general.h"
#include "moptop_server.h"

#include "pirot_command.h"
#include "pirot_general.h"
#include "pirot_setup.h"

/* hash defines */
/**
 * Timezone offset for 1 hour.
 */
#define TIMEZONE_OFFSET_HOUR (3600)
/**
 * Timezone offset for BST.
 */
#define TIMEZONE_OFFSET_BST  (TIMEZONE_OFFSET_HOUR)
/**
 * Timezone offset for HST (roughly!).
 */
#define TIMEZONE_OFFSET_HST  (-10*TIMEZONE_OFFSET_HOUR)

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/* internal functions */
static int Command_Parse_Date(char *time_string,int *time_secs);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Handle a command of the form: "abort".
 * <ul>
 * <li>We abort any running multrun's by calling Moptop_Multrun_Abort.
 * <li>We abort any running bias/dark commands's by calling Moptop_Bias_Dark_Abort.
 * <li>We check the returned values from the aborts to see if they failed, ang log/return an error is this is the case.
 * <li>Otherwise we set the reply_string to a successful message.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_multrun.html#Moptop_Multrun_Abort
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Abort
 */
int Moptop_Command_Abort(char *command_string,char **reply_string)
{
	int multrun_abort_retval, bias_dark_abort_retval;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* abort multrun */
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_INTERMEDIATE,
			   "COMMAND","Aborting multrun.");
#endif
	multrun_abort_retval = Moptop_Multrun_Abort();
#if MOPTOP_DEBUG > 5
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_INTERMEDIATE,
			   "COMMAND","Aborting bias/dark.");
#endif
	/* abort bias/dark */
	bias_dark_abort_retval = Moptop_Bias_Dark_Abort();
	/* check to see if there were problems with the aborts */
	if((multrun_abort_retval == FALSE)||(bias_dark_abort_retval == FALSE))
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Abort",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Abort",
					  LOG_VERBOSITY_TERSE,"COMMAND","Failed to abort multrun (%d) or bias/dark (%d) command.",
					  multrun_abort_retval,bias_dark_abort_retval);
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to abort multrun/bias/dark command."))
			return FALSE;
		return TRUE;
	}
	if(!Moptop_General_Add_String(reply_string,"0 Multrun/Bias/Dark aborted."))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle config commands of the forms:
 * <ul>
 * <li>"config bin <bin>"
 * <li>"config filter <filtername>"
 * <li>"config rotorspeed <slow|fast>"
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_config.html#Moptop_Config_Rotator_Is_Enabled
 * @see moptop_config.html#Moptop_Config_Filter_Wheel_Is_Enabled
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_general.html#Moptop_General_Add_Integer_To_String
 * @see moptop_multrun.html#Moptop_Multrun_Exposure_Length_Set
 * @see moptop_multrun.html#Moptop_Multrun_Filter_Name_Set
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Speed_Set
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Run_Velocity_Set
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Step_Angle_Set
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Run_Velocity_Get
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Step_Angle_Get
 * @see ../ccd/cdocs/ccd_setup.html#CCD_Setup_Dimensions
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Name_To_Position
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Move
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Rotator_Run_Velocity
 * @see ../pirot/cdocs/pirot_setup.html#PIROT_Setup_Trigger_Step_Angle
 */
int Moptop_Command_Config(char *command_string,char **reply_string)
{
	int retval,bin,parameter_index,filter_position;
	double camera_exposure_length;
	char filter_string[32];
	char rotor_speed_string[32];
	char sub_config_command_string[16];

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"config %15s %n",sub_config_command_string,&parameter_index);
	if(retval != 1)
	{
		Moptop_General_Error_Number = 501;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse config command."))
			return FALSE;
		return TRUE;
	}
#if MOPTOP_DEBUG > 9
	Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_VERY_VERBOSE,
				  "COMMAND","Sub config command string: %s, parameter index %d.",
				  sub_config_command_string,parameter_index);
#endif
        if(strcmp(sub_config_command_string,"bin") == 0)
	{
		retval = sscanf(command_string+parameter_index,"%d",&bin);
		if(retval != 1)
		{
			Moptop_General_Error_Number = 506;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
				"Failed to parse command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse config bin command."))
				return FALSE;
			return TRUE;
		}
		/* configure CCD binning */
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting binning to: %d.",bin);
#endif
		if(!CCD_Setup_Dimensions(bin))
		{
			Moptop_General_Error_Number = 507;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Config:Failed to set CCD binning to %d.",
				bin);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
						  LOG_VERBOSITY_TERSE,"COMMAND","Failed to set CCD binning to %d.",
						  bin);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to set CCD binning to:"))
				return FALSE;
			if(!Moptop_General_Add_Integer_To_String(reply_string,bin))
				return FALSE;
			return TRUE;
		}
		if(!Moptop_General_Add_String(reply_string,"0 CCD Binning set to:"))
			return FALSE;
		if(!Moptop_General_Add_Integer_To_String(reply_string,bin))
			return FALSE;
	}
	else if(strcmp(sub_config_command_string,"filter") == 0)
	{
		/* copy rest of command as filter name - filter names have spaces in them! */
		strncpy(filter_string,command_string+parameter_index,31);
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting Filter to: %s.",filter_string);
#endif
		if(Moptop_Config_Filter_Wheel_Is_Enabled())
		{
			/* string to position conversion */
			if(!Filter_Wheel_Config_Name_To_Position(filter_string,&filter_position))
			{
				Moptop_General_Error_Number = 503;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
					"Failed to convert filter name '%s' to a valid filter position.",filter_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to convert filter name '%s' to a valid filter position.",
							  filter_string);
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to convert filter name:"))
					return FALSE;
				if(!Moptop_General_Add_String(reply_string,filter_string))
					return FALSE;
				return TRUE;
			}
#if MOPTOP_DEBUG > 9
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filter position: %d.",filter_position);
#endif
			/* actually move filter wheel */
			if(!Filter_Wheel_Command_Move(filter_position))
			{
				Moptop_General_Error_Number = 504;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
					"Failed to move filter wheel to filter '%s', position %d.",
					filter_string,filter_position);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to move filter wheel to filter '%s', position %d.",
							  filter_string,filter_position);
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to move filter wheel to filter:"))
					return FALSE;
				if(!Moptop_General_Add_String(reply_string,filter_string))
					return FALSE;
				return TRUE;
			}
			/* success */
			if(!Moptop_General_Add_String(reply_string,"0 Filter wheel moved to position:"))
				return FALSE;
			if(!Moptop_General_Add_String(reply_string,filter_string))
				return FALSE;
		}
		else // filter wheel is not enabled for this C layer
		{
			if(!Moptop_Multrun_Filter_Name_Set(filter_string))
			{
				Moptop_General_Error_Number = 502;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
					"Failed to cache filter name data '%s'.",filter_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to cache filter name data '%s'.",
							  filter_string);
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to cache filter name:"))
					return FALSE;
				if(!Moptop_General_Add_String(reply_string,filter_string))
					return FALSE;
				return TRUE;
			}
			/* success */
			if(!Moptop_General_Add_String(reply_string,"0 Filter :"))
				return FALSE;
			if(!Moptop_General_Add_String(reply_string,filter_string))
				return FALSE;
			if(!Moptop_General_Add_String(reply_string," data cached."))
				return FALSE;
		}
	}
	else if(strcmp(sub_config_command_string,"rotorspeed") == 0)
	{
		retval = sscanf(command_string+parameter_index,"%31s",rotor_speed_string);
		if(retval != 1)
		{
			Moptop_General_Error_Number = 533;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Config:"
				"Failed to parse command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse config bin command."))
				return FALSE;
			return TRUE;
		}
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting rotor speed to: %s.",
					  rotor_speed_string);
#endif
		/* cache the rotator speed setting in the multrun data for FITS header generation */
		Moptop_Multrun_Rotator_Speed_Set(rotor_speed_string);
		if(strcmp(rotor_speed_string,"fast") == 0)
		{
			/* fast speed rotor: 45 deg/s (7.5 rpm) , 16 triggers per rotation, exposure length 0.4s 
			** 0.4 s /((60 s /7.5 rpm )/16 triggers) * 22.5 deg = 18 deg 
			** i.e. exposing for 18deg out of 22.5deg */
			Moptop_Multrun_Rotator_Run_Velocity_Set(45.0);
			Moptop_Multrun_Rotator_Step_Angle_Set(PIROT_SETUP_TRIGGER_STEP_ANGLE_16);
			camera_exposure_length = 0.4;
		}
		else if(strcmp(rotor_speed_string,"slow") == 0)
		{
			/* slow speed rotor: 4.5 deg/s (0.75 rpm), 16 triggers per rotation, exposure length 4.0s 
			** 4.0 s /((60 s /0.75 rpm )/16 triggers) * 22.5 deg = 18 deg 
			** i.e. exposing for 18deg out of 22.5deg */
			Moptop_Multrun_Rotator_Run_Velocity_Set(4.5);
			Moptop_Multrun_Rotator_Step_Angle_Set(PIROT_SETUP_TRIGGER_STEP_ANGLE_16);
			camera_exposure_length = 4.0;
		}
		else
		{
			Moptop_General_Error_Number = 534;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Config:Unknown rotor speed %s.",
				rotor_speed_string);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
						  LOG_VERBOSITY_TERSE,"COMMAND","finished (Unknown rotor speed %s).",
						  rotor_speed_string);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse config rotorspeed command:"))
				return FALSE;
			if(!Moptop_General_Add_String(reply_string,command_string))
				return FALSE;
			return TRUE;
		}
		/* configure rotator if enabled */
		if(Moptop_Config_Rotator_Is_Enabled())
		{
#if MOPTOP_DEBUG > 5
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
						  LOG_VERBOSITY_VERBOSE,"COMMAND",
						  "Setting rotor run velocity to: %.3f.",
						  Moptop_Multrun_Rotator_Run_Velocity_Get());
#endif
			if(!PIROT_Setup_Rotator_Run_Velocity(Moptop_Multrun_Rotator_Run_Velocity_Get()))
			{
				Moptop_General_Error_Number = 535;
				sprintf(Moptop_General_Error_String,
					"Moptop_Command_Config:Failed to set rotator velocity to %.2f deg/s.",
					Moptop_Multrun_Rotator_Run_Velocity_Get());
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "finished (Failed to set rotator velocity to %.2f deg/s).",
							  Moptop_Multrun_Rotator_Run_Velocity_Get());
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to set rotator velocity."))
					return FALSE;
				return TRUE;
			}
#if MOPTOP_DEBUG > 5
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
						  LOG_VERBOSITY_VERBOSE,"COMMAND",
						  "Setting trigger step angle to: %.3f.",
						  Moptop_Multrun_Rotator_Step_Angle_Get());
#endif
			if(!PIROT_Setup_Trigger_Step_Angle(Moptop_Multrun_Rotator_Step_Angle_Get()))
			{
				Moptop_General_Error_Number = 536;
				sprintf(Moptop_General_Error_String,
					"Moptop_Command_Config:Failed to set rotator trigger step angle to %.2f deg.",
					Moptop_Multrun_Rotator_Step_Angle_Get());
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
						      "finished (Failed to set rotator trigger step angle %.2f deg).",
							  Moptop_Multrun_Rotator_Step_Angle_Get());
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to set rotator trigger step angle."))
					return FALSE;
				return TRUE;
			}
#if MOPTOP_DEBUG > 5
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",
					   LOG_VERBOSITY_VERBOSE,"COMMAND",
					   "Configuring the rotator is now done in multrun_setup.");
#endif
		} /* end if Moptop_Config_Rotator_Is_Enabled */
		/* set CCD exposure length to match */
#if MOPTOP_DEBUG > 5
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting multrun exposure length to: %.3fs.",
					  camera_exposure_length);
#endif
		if(!Moptop_Multrun_Exposure_Length_Set(camera_exposure_length))
		{
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_TERSE,
					     "COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Config",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "finished (Failed to set exposure length to %.2f s).",
						  camera_exposure_length);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to set exposure length."))
				return FALSE;
			return TRUE;
		}
		if(!Moptop_General_Add_String(reply_string,"0 Config rotorspeed completed."))
			return FALSE;
	}
	else
	{
		if(!Moptop_General_Add_String(reply_string,"1 Unknown config sub-command:"))
			return FALSE;
		if(!Moptop_General_Add_String(reply_string,sub_config_command_string))
			return FALSE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Implementation of FITS Header commands.
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_fits_header.html#Moptop_Fits_Header_Logical_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Float_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Integer_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_String_Add
 * @see moptop_fits_header.html#Moptop_Fits_Header_Clear
 * @see moptop_fits_header.html#Moptop_Fits_Header_Delete
 */
int Moptop_Command_Fits_Header(char *command_string,char **reply_string)
{
	char operation_string[8];
	char keyword_string[13];
	char type_string[8];
	char value_string[80];
	int retval,command_string_index,ivalue,value_index;
	double dvalue;

	/* parse command to retrieve operation*/
	retval = sscanf(command_string,"fitsheader %6s %n",operation_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Moptop_General_Error_Number = 517;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
			"Failed to parse command %s (%d).",command_string,retval);
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader command."))
			return FALSE;
		return TRUE;
	}
	/* do operation */
	if(strncmp(operation_string,"add",3) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s %7s %n",keyword_string,type_string,
				&value_index);
		if((retval != 3)&&(retval != 2)) /* %n may or may not increment retval*/
		{
			Moptop_General_Error_Number = 518;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Failed to parse add command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (add command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader add command."))
				return FALSE;
			return TRUE;
		}
		strncpy(value_string,command_string+command_string_index+value_index,79);
		value_string[79] = '\0';
		if(strncmp(type_string,"boolean",7)==0)
		{
			/* parse value */
			if(strncmp(value_string,"true",4) == 0)
				ivalue = TRUE;
			else if(strncmp(value_string,"false",5) == 0)
				ivalue = FALSE;
			else
			{
				Moptop_General_Error_Number = 519;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add boolean command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add boolean command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add boolean command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Logical_Add(keyword_string,ivalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add boolean to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add boolean fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"float",5)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%lf",&dvalue);
			if(retval != 1)
			{
				Moptop_General_Error_Number = 520;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add float command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add float command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add float command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Float_Add(keyword_string,dvalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add float to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add float fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"integer",7)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%d",&ivalue);
			if(retval != 1)
			{
				Moptop_General_Error_Number = 521;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
					"Add integer command had unknown value %s.",value_string);
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add integer command had unknown value %s.",value_string);
#endif
				if(!Moptop_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add integer command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Moptop_Fits_Header_Integer_Add(keyword_string,ivalue,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add integer to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add integer fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"string",6)==0)
		{
			/* do operation */
			if(!Moptop_Fits_Header_String_Add(keyword_string,value_string,NULL))
			{
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add string to FITS header.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to add string fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else
		{
			Moptop_General_Error_Number = 522;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Add command had unknown type %s.",type_string);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","Add command had unknown type %s.",type_string);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader add command type."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"delete",6) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s",keyword_string);
		if(retval != 1)
		{
			Moptop_General_Error_Number = 523;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
				"Failed to parse delete command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (delete command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader delete command."))
				return FALSE;
			return TRUE;
		}
		/* do delete */
		if(!Moptop_Fits_Header_Delete(keyword_string))
		{
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to delete FITS header with keyword '%s'.",keyword_string);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to delete fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"clear",5) == 0)
	{
		/* do clear */
		if(!Moptop_Fits_Header_Clear())
		{
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","Failed to clear FITS header.");
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to clear fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else
	{
		Moptop_General_Error_Number = 524;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Fits_Header:"
			"Unknown operation %s:Failed to parse command %s.",operation_string,command_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Fits_Header",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Fits_Header",
					  LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown operation %s:Failed to parse command %s.",
					  operation_string,command_string);
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse fitsheader command: Unknown operation."))
			return FALSE;
		return TRUE;
	}
	if(!Moptop_General_Add_String(reply_string,"0 FITS Header command succeeded."))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Fits_Header",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a command of the form: "multrun <length> <count> <standard>".
 * <ul>
 * <li>The multrun command is parsed to get the exposure length, count and standard (true|false) values.
 * <li>We call Moptop_Multrun to take the multrun images.
 * <li>The reply string is constructed of the form "0 <filename count> <multrun number> <last FITS filename>".
 * <li>We log the returned filenames.
 * <li>We free the returned filenames.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_multrun.html#Moptop_Multrun
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Free
 */
int Moptop_Command_Multrun(char *command_string,char **reply_string)
{
	struct timespec start_time = {0L,0L};
	char **filename_list = NULL;
	char standard_string[8];
	char count_string[16];
	int i,retval,exposure_length,exposure_count,filename_count,do_standard,multrun_number;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"multrun %d %d %7s",&exposure_length,&exposure_count,standard_string);
	if(retval != 3)
	{
		Moptop_General_Error_Number = 505;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse multrun command."))
			return FALSE;
		return TRUE;
	}
	/* parse standard string */
	if(strcmp(standard_string,"true") == 0)
		do_standard = TRUE;
	else if(strcmp(standard_string,"false") == 0)
		do_standard = FALSE;
	else
	{
		Moptop_General_Error_Number = 539;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:Illegal standard value '%s'.",
			standard_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed:Illegal standard value."))
			return FALSE;
		return TRUE;
	}
	/* do multrun */
	retval = Moptop_Multrun(exposure_length,(exposure_length > 0),exposure_count,(exposure_count > 0),do_standard,
				&filename_list,&filename_count);
	if(retval == FALSE)
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",
				   LOG_VERBOSITY_TERSE,"COMMAND","Multrun failed.");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,"0 "))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add number of FITS images */
	sprintf(count_string,"%d ",filename_count);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* get multrun number */
	multrun_number = CCD_Fits_Filename_Multrun_Get();
	sprintf(count_string,"%d ",multrun_number);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add last filename */
	if(filename_count > 0)
	{
		if(!Moptop_General_Add_String(reply_string,filename_list[filename_count-1]))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	else
	{
		if(!Moptop_General_Add_String(reply_string,"none"))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	/* log filenames returned */
	for(i=0; i < filename_count; i++)
	{
#if MOPTOP_DEBUG > 8
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Multrun",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filename %d : %s",i,filename_list[i]);
#endif
	}
	if(!CCD_Fits_Filename_List_Free(&filename_list,&filename_count))
	{
		Moptop_General_Error_Number = 510;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Multrun:CCD_Fits_Filename_List_Free failed.");
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 Multrun failed (freeing filename list)."))
			return FALSE;
		return TRUE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Routine to implement the "multrun_setup" command. This is used to set everything up
 * the rotator is started.
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_general.html#Moptop_General_Add_Integer_To_String
 * @see moptop_multrun.html#Moptop_Multrun_Setup
 */
int Moptop_Command_Multrun_Setup(char *command_string,char **reply_string)
{
	int retval,multrun_number;
	
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun_Setup",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	retval = Moptop_Multrun_Setup(&multrun_number);
	if(retval == FALSE)
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Multrun_Setup",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun_Setup",
				   LOG_VERBOSITY_TERSE,"COMMAND","Multrun setup failed.");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Multrun setup failed."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,"0 "))
		return FALSE;
	if(!Moptop_General_Add_Integer_To_String(reply_string,multrun_number))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Multrun_Setup",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;	
}

/**
 * Handle a command of the form: "multbias <count>".
 * <ul>
 * <li>The multbias command is parsed to get the exposure count value.
 * <li>We call Moptop_Bias_Dark_MultBias to create the bias frames.
 * <li>The reply string is constructed of the form "0 <filename count> <multrun number> <last FITS filename>".
 * <li>We log the returned filenames.
 * <li>We free the returned filenames.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_MultBias
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Free
 */
int Moptop_Command_MultBias(char *command_string,char **reply_string)
{
	char **filename_list = NULL;
	char count_string[16];
	int i,retval,exposure_count,filename_count,multrun_number;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultBias",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"multbias %d",&exposure_count);
	if(retval != 1)
	{
		Moptop_General_Error_Number = 527;
		sprintf(Moptop_General_Error_String,"Moptop_Command_MultBias:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultBias",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultBias",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse multbias command."))
			return FALSE;
		return TRUE;
	}
	/* do multbias */
	retval = Moptop_Bias_Dark_MultBias(exposure_count,&filename_list,&filename_count);
	if(retval == FALSE)
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultBias",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultBias",
				   LOG_VERBOSITY_TERSE,"COMMAND","MultBias failed.");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 MultBias failed."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,"0 "))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add number of FITS images */
	sprintf(count_string,"%d ",filename_count);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* get multrun number */
	multrun_number = CCD_Fits_Filename_Multrun_Get();
	sprintf(count_string,"%d ",multrun_number);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add last filename */
	if(filename_count > 0)
	{
		if(!Moptop_General_Add_String(reply_string,filename_list[filename_count-1]))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	else
	{
		if(!Moptop_General_Add_String(reply_string,"none"))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	/* log filenames returned */
	for(i=0; i < filename_count; i++)
	{
#if MOPTOP_DEBUG > 8
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_MultBias",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filename %d : %s",i,filename_list[i]);
#endif
	}
	if(!CCD_Fits_Filename_List_Free(&filename_list,&filename_count))
	{
		Moptop_General_Error_Number = 537;
		sprintf(Moptop_General_Error_String,"Moptop_Command_MultBias:CCD_Fits_Filename_List_Free failed.");
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultBias",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 MultBias failed (freeing filename list)."))
			return FALSE;
		return TRUE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultBias",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a command of the form: "multdark <exposure length> <count>".
 * <ul>
 * <li>The multdark command is parsed to get the exposure length and exposure count value.
 * <li>We call Moptop_Bias_Dark_MultDark to create the dark frames.
 * <li>The reply string is constructed of the form "0 <filename count> <multrun number> <last FITS filename>".
 * <li>We log the returned filenames.
 * <li>We free the returned filenames.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_MultDark
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_List_Free
 */
int Moptop_Command_MultDark(char *command_string,char **reply_string)
{
	char **filename_list = NULL;
	char count_string[16];
	int i,retval,exposure_length,exposure_count,filename_count,multrun_number;

#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultDark",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"multdark %d %d",&exposure_length,&exposure_count);
	if(retval != 2)
	{
		Moptop_General_Error_Number = 538;
		sprintf(Moptop_General_Error_String,"Moptop_Command_MultDark:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultDark",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultDark",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse multdark command."))
			return FALSE;
		return TRUE;
	}
	/* do multbias */
	retval = Moptop_Bias_Dark_MultDark(exposure_length,exposure_count,&filename_list,&filename_count);
	if(retval == FALSE)
	{
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultDark",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultDark",
				   LOG_VERBOSITY_TERSE,"COMMAND","MultDark failed.");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 MultDark failed."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,"0 "))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add number of FITS images */
	sprintf(count_string,"%d ",filename_count);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* get multrun number */
	multrun_number = CCD_Fits_Filename_Multrun_Get();
	sprintf(count_string,"%d ",multrun_number);
	if(!Moptop_General_Add_String(reply_string,count_string))
	{
		CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add last filename */
	if(filename_count > 0)
	{
		if(!Moptop_General_Add_String(reply_string,filename_list[filename_count-1]))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	else
	{
		if(!Moptop_General_Add_String(reply_string,"none"))
		{
			CCD_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	/* log filenames returned */
	for(i=0; i < filename_count; i++)
	{
#if MOPTOP_DEBUG > 8
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_MultDark",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filename %d : %s",i,filename_list[i]);
#endif
	}
	if(!CCD_Fits_Filename_List_Free(&filename_list,&filename_count))
	{
		Moptop_General_Error_Number = 544;
		sprintf(Moptop_General_Error_String,"Moptop_Command_MultDark:CCD_Fits_Filename_List_Free failed.");
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_MultDark",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Moptop_General_Add_String(reply_string,"1 MultDark failed (freeing filename list)."))
			return FALSE;
		return TRUE;
	}
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_MultDark",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a status command. Possible forms: 
 * <ul>
 * <li>status temperature [get|status]
 * <li>status filterwheel [filter|position|status]
 * <li>status rotator [position|speed|status]
 * <li>status exposure [status|count|length|start_time]
 * <li>status exposure [index|multrun|run|window]
 * <li>status fits_instrument_code
 * </ul>
 * <ul>
 * <li>The status command is parsed to retrieve the subsystem (1st parameter).
 * <li>Based on the subsystem, further parsing occurs.
 * <li>The relevant status is retrieved, and a suitable reply constructed.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_In_Progress
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Count_Get
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Per_Frame_Exposure_Length_Get
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Exposure_Start_Time_Get
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Exposure_Index_Get
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Multrun_Get
 * @see moptop_bias_dark.html#Moptop_Bias_Dark_Run_Get
 * @see moptop_config.html#Moptop_Config_Filter_Wheel_Is_Enabled
 * @see moptop_general.html#Moptop_General_Log
 * @see moptop_general.html#Moptop_General_Error_Number
 * @see moptop_general.html#Moptop_General_Error_String
 * @see moptop_general.html#Moptop_General_Add_String
 * @see moptop_general.html#Moptop_General_Get_Time_String
 * @see moptop_general.html#Moptop_General_Get_Current_Time_String
 * @see moptop_multrun.html#Moptop_Multrun_Rotator_Speed_Get
 * @see moptop_multrun.html#Moptop_Multrun_In_Progress
 * @see moptop_multrun.html#Moptop_Multrun_Count_Get
 * @see moptop_multrun.html#Moptop_Multrun_Per_Frame_Exposure_Length_Get
 * @see moptop_multrun.html#Moptop_Multrun_Exposure_Start_Time_Get
 * @see moptop_multrun.html#Moptop_Multrun_Exposure_Index_Get
 * @see moptop_multrun.html#Moptop_Multrun_Multrun_Get
 * @see moptop_multrun.html#Moptop_Multrun_Run_Get
 * @see moptop_multrun.html#Moptop_Multrun_Window_Get
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_Exposure_Status_To_String
 * @see ../ccd/cdocs/ccd_exposure.html#CCD_EXPOSURE_TRIGGER_MODE
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Multrun_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Run_Get
 * @see ../ccd/cdocs/ccd_fits_filename.html#CCD_Fits_Filename_Window_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Temperature_Status_String
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Cached_Temperature
 * @see ../ccd/cdocs/ccd_temperature.html#CCD_Temperature_Get_Cached_Temperature_Status_String
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 * @see ../pirot/cdocs/pirot_command.html#PIROT_Command_Query_POS
 */
int Moptop_Command_Status(char *command_string,char **reply_string)
{
	struct timespec status_time;
	char time_string[32];
	char return_string[128];
	char subsystem_string[32];
	char get_set_string[16];
	char key_string[64];
	char temperature_status_string[32];
	char filter_name_string[32];
	char rotator_speed_string[32];
	char instrument_code;
	char *camera_name_string = NULL;
	int retval,command_string_index,ivalue,filter_wheel_position,rotator_on_target;
	double temperature,rotator_position;
	
	/* parse command */
	retval = sscanf(command_string,"status %31s %n",subsystem_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Moptop_General_Error_Number = 511;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
			"Failed to parse command %s (%d).",command_string,retval);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	/* initialise return string */
	strcpy(return_string,"0 ");
	/* parse subsystem */
	if(strncmp(subsystem_string,"exposure",8) == 0)
	{
		if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			if(Moptop_Multrun_In_Progress()||Moptop_Bias_Dark_In_Progress())
				strcat(return_string,"true");
			else
				strcat(return_string,"false");
		}
		else if(strncmp(command_string+command_string_index,"count",5)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = Moptop_Bias_Dark_Count_Get();
			else
				ivalue = Moptop_Multrun_Count_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"length",6)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = Moptop_Bias_Dark_Per_Frame_Exposure_Length_Get();
			else
				ivalue = Moptop_Multrun_Per_Frame_Exposure_Length_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"start_time",10)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				Moptop_Bias_Dark_Exposure_Start_Time_Get(&status_time);
			else
				Moptop_Multrun_Exposure_Start_Time_Get(&status_time);
			Moptop_General_Get_Time_String(status_time,time_string,31);
			sprintf(return_string+strlen(return_string),"%s",time_string);
		}
		else if(strncmp(command_string+command_string_index,"index",5)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = Moptop_Bias_Dark_Exposure_Index_Get();
			else
				ivalue = Moptop_Multrun_Exposure_Index_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"multrun",7)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = Moptop_Bias_Dark_Multrun_Get();
			else
				ivalue = Moptop_Multrun_Multrun_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"run",3)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = Moptop_Bias_Dark_Run_Get();
			else
				ivalue = Moptop_Multrun_Run_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"window",6)==0)
		{
			if(Moptop_Bias_Dark_In_Progress())
				ivalue = 0; /* bias/darks do not use windows */
			else
				ivalue = Moptop_Multrun_Window_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else
		{
			Moptop_General_Error_Number = 512;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse exposure command %s.",command_string+command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND","Failed to parse exposure command %s.",
						  command_string+command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse exposure status command."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(subsystem_string,"filterwheel",11) == 0)
	{
		if(Moptop_Config_Filter_Wheel_Is_Enabled())
		{
			if(!Filter_Wheel_Command_Get_Position(&filter_wheel_position))
			{
				Moptop_General_Error_Number = 509;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
					"Failed to get filter wheel position.");
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
						   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get filter wheel position.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to get filter wheel position."))
					return FALSE;
				return TRUE;
			}/* end if filter wheel is enabled */
		}
		else
		{
#if MOPTOP_DEBUG > 5
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_VERBOSE,"COMMAND",
					   "Moptop filter wheel is NOT enabled, faking filter wheel position to 0 (moving).");
#endif
			/* we pretend the filter wheel is moving when it is not enabled */
			filter_wheel_position = 0;
		}
		if(strncmp(command_string+command_string_index,"filter",6)==0)
		{
			if(filter_wheel_position == 0) /* moving */
			{
				strcpy(filter_name_string,"moving");
			}
			else
			{
				if(!Filter_Wheel_Config_Position_To_Name(filter_wheel_position,filter_name_string))
				{
					Moptop_General_Error_Number = 514;
					sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
						"Failed to get filter wheel filter name from position %d.",
						filter_wheel_position);
					Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
					Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
								  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to get filter wheel filter name from position %d.",
								  filter_wheel_position);
#endif
					if(!Moptop_General_Add_String(reply_string,
							"1 Failed to get filter wheel filter name from position:"))
						return FALSE;
					if(!Moptop_General_Add_Integer_To_String(reply_string,filter_wheel_position))
						return FALSE;
					return TRUE;
				}
			}
			strcat(return_string,filter_name_string);
		}
		else if(strncmp(command_string+command_string_index,"position",8)==0)
		{
			sprintf(return_string+strlen(return_string),"%d",filter_wheel_position);
		}
		else if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			if(filter_wheel_position == 0)/* moving */
				strcat(return_string,"moving");
			else
				strcat(return_string,"in_position");
		}
		else
		{
			Moptop_General_Error_Number = 525;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse filterwheel command %s.",command_string+command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse filterwheel command %s.",
						  command_string+command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse filterwheel status command."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(subsystem_string,"fits_instrument_code",20) == 0)
	{
		sprintf(key_string,"file.fits.instrument_code");
		if(!Moptop_Config_Get_Character(key_string,&instrument_code))
		{
			Moptop_General_Error_Number = 546;
			sprintf(Moptop_General_Error_String,
			       "Moptop_Command_Status:Failed to get instrument code config.");
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (failed to get instrument code).");
#endif
			if(!Moptop_General_Add_String(reply_string,
						      "1 Failed to get instrument code."))
				return FALSE;
			return FALSE;
		}
		sprintf(return_string+strlen(return_string),"%c",instrument_code);
	}
	else if(strncmp(subsystem_string,"rotator",7) == 0)
	{
		if(strncmp(command_string+command_string_index,"position",8)==0)
		{
			if(!PIROT_Command_Query_POS(&rotator_position))
			{
				Moptop_General_Error_Number = 541;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
					"Failed to query rotator position.");
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to query rotator position.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to query rotator position."))
					return FALSE;
				return TRUE;
			}
			sprintf(return_string+strlen(return_string),"%.2f",rotator_position);			
		}
		else if(strncmp(command_string+command_string_index,"speed",5)==0)
		{
			Moptop_Multrun_Rotator_Speed_Get(rotator_speed_string);
			strcat(return_string,rotator_speed_string);
		}
		else if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			if(!PIROT_Command_Query_ONT(&rotator_on_target))
			{
				Moptop_General_Error_Number = 542;
				sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
					"Failed to query rotator on target.");
				Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
				Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to query rotator on target.");
#endif
				if(!Moptop_General_Add_String(reply_string,"1 Failed to query rotator on target."))
					return FALSE;
				return TRUE;
			}
			if(rotator_on_target)
				strcat(return_string,"stopped");
			else
				strcat(return_string,"moving");
		}
		else
		{
			Moptop_General_Error_Number = 543;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse rotator command %s.",command_string+command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse rotator command %s.",
						  command_string+command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse status rotator command."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(subsystem_string,"temperature",11) == 0)
	{
		retval = sscanf(command_string,"status temperature %15s",get_set_string);
		if(retval != 1)
		{
			Moptop_General_Error_Number = 526;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse command %s (%d).",command_string,retval);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Moptop_General_Add_String(reply_string,
						      "1 Failed to parse status temperature ."))
				return FALSE;
			return TRUE;
		}
		/* check subcommand */
		if(strncmp(get_set_string,"get",3)==0)
		{
			if((Moptop_Multrun_In_Progress() == FALSE)&&(Moptop_Bias_Dark_In_Progress() == FALSE))
			{
				if(!CCD_Temperature_Get(&temperature))
				{
					Moptop_General_Error_Number = 513;
					sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
						"Failed to get temperature.");
					Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
					Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
							   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get temperature.");
#endif
					if(!Moptop_General_Add_String(reply_string,"1 Failed to get temperature."))
						return FALSE;
					return TRUE;
				}
				Moptop_General_Get_Current_Time_String(time_string,31);
			}
			else
			{
				CCD_Temperature_Get_Cached_Temperature(&temperature,&status_time);
				Moptop_General_Get_Time_String(status_time,time_string,31);
			}
			sprintf(return_string+strlen(return_string),"%s %.2f",time_string,temperature);
		}
		else if(strncmp(get_set_string,"status",6)==0)
		{
			if((Moptop_Multrun_In_Progress() == FALSE)&&(Moptop_Bias_Dark_In_Progress() == FALSE))
			{
				if(!CCD_Temperature_Get_Temperature_Status_String(temperature_status_string,31))
				{
					Moptop_General_Error_Number = 508;
					sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
						"Failed to get temperature status.");
					Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
					Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",
							   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get temperature.");
#endif
					if(!Moptop_General_Add_String(reply_string,
								      "1 Failed to get temperature status."))
					{
						return FALSE;
					}
					return TRUE;
				}
				Moptop_General_Get_Current_Time_String(time_string,31);
			}
			else
			{
				CCD_Temperature_Get_Cached_Temperature_Status_String(temperature_status_string,
										     &status_time);
				Moptop_General_Get_Time_String(status_time,time_string,31);				
			}
			sprintf(return_string+strlen(return_string),"%s %s",time_string,temperature_status_string);
		}
		else
		{
			Moptop_General_Error_Number = 515;
			sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
				"Failed to parse temperature command %s from %d.",command_string,command_string_index);
			Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
			Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse temperature command %s from %d.",
						  command_string,command_string_index);
#endif
			if(!Moptop_General_Add_String(reply_string,"1 Failed to parse temperature status command."))
				return FALSE;
			return TRUE;
		}
	}
	else
	{
		Moptop_General_Error_Number = 516;
		sprintf(Moptop_General_Error_String,"Moptop_Command_Status:"
			"Unknown subsystem %s:Failed to parse command %s.",subsystem_string,command_string);
		Moptop_General_Error("command","moptop_command.c","Moptop_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if MOPTOP_DEBUG > 1
		Moptop_General_Log_Format("command","moptop_command.c","Moptop_Command_Status",LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown subsystem %s:Failed to parse command %s.",
					  subsystem_string,command_string);
#endif
		if(!Moptop_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Moptop_General_Add_String(reply_string,return_string))
		return FALSE;
#if MOPTOP_DEBUG > 1
	Moptop_General_Log("command","moptop_command.c","Moptop_Command_Status",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Parse a date of the form "2007-05-03T07:38:48.099 UTC" into number of seconds since 1970 (unix time).
 * @param time_string The string.
 * @param time_secs The address of an integer to store the number of seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #TIMEZONE_OFFSET_BST
 * @see #TIMEZONE_OFFSET_HST
 */
static int Command_Parse_Date(char *time_string,int *time_secs)
{
	struct tm time_data;
	int year,month,day,hours,minutes,retval;
	double seconds;
	char timezone_string[16];
	time_t time_in_secs;

	/* check parameters */
	if(time_string == NULL)
	{
		Moptop_General_Error_Number = 528;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:time_string was NULL.");
		return FALSE;
	}
	if(time_secs == NULL)
	{
		Moptop_General_Error_Number = 529;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:time_secs was NULL.");
		return FALSE;
	}
#if MOPTOP_DEBUG > 9
	Moptop_General_Log_Format("command","moptop_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,
				  "COMMAND","Parsing date/time '%s'.",time_string);
#endif
	/* parse time_string into fields */
	strcpy(timezone_string,"UTC");
	retval = sscanf(time_string,"%d-%d-%d T %d:%d:%lf %15s",&year,&month,&day,
			&hours,&minutes,&seconds,timezone_string);
	if(retval < 6)
	{
		Moptop_General_Error_Number = 530;
		sprintf(Moptop_General_Error_String,
			"Command_Parse_Date:Failed to parse '%s', only parsed %d fields: year=%d,month=%d,day=%d,"
			"hour=%d,minute=%d,second=%.2f,timezone_string=%s.",time_string,retval,year,month,day,
			hours,minutes,seconds,timezone_string);
		return FALSE;
	}
#if MOPTOP_DEBUG > 9
	Moptop_General_Log_Format("command","moptop_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,"COMMAND",
			    "Date/time '%s' has year=%d,month=%d,day=%d,hour=%d,minute=%d,seconds=%.2lf,timezone=%s.",
				  time_string,year,month,day,hours,minutes,seconds,timezone_string);
#endif
	/* construct tm */
	time_data.tm_year  = year-1900; /* years since 1900 */
	time_data.tm_mon = month-1; /* 0..11 */
	time_data.tm_mday  = day; /* 1..31 */
	time_data.tm_hour  = hours; /* 0..23 */
	time_data.tm_min   = minutes;
	time_data.tm_sec   = seconds;
	time_data.tm_wday  = 0;
	time_data.tm_yday  = 0;
	time_data.tm_isdst = 0;
	/* BSD extension stuff */
	/*
	time_data.tm_gmtoff = 0;
	time_data.tm_zone = strdup(timezone_string);
	*/
	/* create time in UTC */
	time_in_secs = mktime(&time_data);
	if(time_in_secs < 0)
	{
		Moptop_General_Error_Number = 532;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:mktime failed.");
		return FALSE;
	}
	(*time_secs) = (int)time_in_secs;
	if(strcmp(timezone_string,"UTC") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"GMT") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"BST") == 0)
	{
		(*time_secs) += TIMEZONE_OFFSET_BST;
	}
	else if(strcmp(timezone_string,"HST") == 0)
	{
		(*time_secs) += TIMEZONE_OFFSET_HST;
	}
	else
	{
		Moptop_General_Error_Number = 531;
		sprintf(Moptop_General_Error_String,"Command_Parse_Date:Unknown timezone '%s'.",timezone_string);
		return FALSE;
	}
	return TRUE;
}
