/* pirot_command.c
** PI Rotator command routines
** $Header$
*/
/**
 * PI Rotator command routines.
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
#include "PI_GCS2_DLL.h"
#include "log_udp.h"
#include "pirot_general.h"
#include "pirot_command.h"
#include "pirot_usb.h"

/**
 * Length to use for character arrays.
 */
#define STRING_LENGTH                (256)
/**
 * Default rotator tolerance in degrees.
 */
#define DEFAULT_ROTATOR_TOLERANCE    (0.004)
/**
 * The default name of the rotator axis. Used by the PI rotator library to query and move the rotator.
 */
#define COMMAND_ROTATOR_AXIS         ("1")
/**
 * The maximum allowed position the rotator can be commanded to.
 */
#define COMMAND_MOV_POSITION_MAX     (36000.0)
/**
 * The maximum allowed velocity of the rotator.
 */
#define COMMAND_ROTATOR_VELOCITY_MAX (360.0)

/* structures */

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see #PIROT_ERROR_STRING_LENGTH
 */
static char Command_Error_String[PIROT_ERROR_STRING_LENGTH] = "";


/* =======================================
**  external functions 
** ======================================= */
/**
 * Generic routine to send a string command to the rotator. This is best used
 * for commands that expect no reply, otherwise extra routines need to be called to NOT confuse the controller - 
 * see the manual for details.
 * @param command_string A string containing the command to send.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command(char *command_string)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command: Started.");
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 15;
		sprintf(Command_Error_String,"PIROT_Command: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command: PI_GcsCommandset(usb_id=%d,command_string=%s).",
			 PIROT_USB_Get_ID(),command_string);
#endif /* LOGGING */
	retval = PI_GcsCommandset(PIROT_USB_Get_ID(),command_string);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 16;
		sprintf(Command_Error_String,"PIROT_Command: PI_GcsCommandset failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 17;
		sprintf(Command_Error_String,"PIROT_Command: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Configures the trigger output conditions for the digital output line ID '1'.
 * Uses the PI rotator library "PI_CTO" routine / "CTO" command. The trigger output ID is always '1' in this routine.
 * @param trigger_parameter Which trigger parameter to alter. See the manual(C-867-1U-UserManual-MS223E200.pdf), P171,
 * as follows:
 *        <ul>
 *        <li>1 = TriggerStep
 *        <li>2 = Axis
 *        <li>3 = TriggerMode
 *        <li>7 = Polarity
 *        <li>8 = StartThreshold
 *        <li>9 = StopThreshold
 *        <li>10 = TriggerPosition
 *        <li>11 = PulseWidth
 *        </ul>
 *        These are encoded into the PIROT_COMMAND_CTO_PARAMETER_ENUM enumeration.
 * @param value The value to set the given parameter to.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #PIROT_COMMAND_CTO_PARAMETER_ENUM
 * @see #PIROT_COMMAND_IS_CTO_PARAMETER
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_IS_BOOLEAN
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_CTO(enum PIROT_COMMAND_CTO_PARAMETER_ENUM trigger_parameter,double value)
{
	int trigger_output_id_list[1];
	int trigger_parameter_list[1];
	double trigger_value_list[1];
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_CTO(trigger_parameter=%d,value=%.2f): Started.",
			 trigger_parameter,value);
#endif /* LOGGING */
	if(!PIROT_COMMAND_IS_CTO_PARAMETER(trigger_parameter))
	{
		Command_Error_Number = 44;
		sprintf(Command_Error_String,"PIROT_Command_CTO: Invalid trigger parameter %d.",trigger_parameter);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 41;
		sprintf(Command_Error_String,"PIROT_Command_CTO: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	trigger_output_id_list[0] = 1;
	trigger_parameter_list[0] = trigger_parameter;
	trigger_value_list[0] = value;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Command_CTO: PI_CTO(usb_id=%d,trigger_output_id_list[0]=%d,"
			 "trigger_parameter_list[0]=%d,trigger_value_list[0]=%.2f).",
			 PIROT_USB_Get_ID(),trigger_output_id_list[0],trigger_parameter_list[0],trigger_value_list[0]);
#endif /* LOGGING */
	retval = PI_CTO(PIROT_USB_Get_ID(),trigger_output_id_list,trigger_parameter_list,trigger_value_list,1);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 42;
		sprintf(Command_Error_String,"PIROT_Command_CTO: PI_CTO failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 43;
		sprintf(Command_Error_String,"PIROT_Command_CTO: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_CTO: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Fast move to ReFerence switch command. 
 * This moves the rotator to a known physical reference point and the current position is set to the known reference point. 
 * Uses the PI rotator library "PI_FRF" routine/FRF command.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_FRF(void)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_FRF(): Started.");
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 38;
		sprintf(Command_Error_String,"PIROT_Command_FRF: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_FRF: PI_FRF(usb_id=%d,axes=%s).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS);
#endif /* LOGGING */
	retval = PI_FRF(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 39;
		sprintf(Command_Error_String,"PIROT_Command_FRF: PI_FRF failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 40;
		sprintf(Command_Error_String,"PIROT_Command_FRF: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_FRF: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * MOVe the rotator. Uses the PI rotator library "PI_MOV" routine.
 * @param position The target position to move the rotator to, in degrees (0..COMMAND_MOV_POSITION_MAX).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #COMMAND_MOV_POSITION_MAX
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_MOV(double position)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_MOV(position=%.2f): Started.",position);
#endif /* LOGGING */
	if((position < 0.0) || (position > COMMAND_MOV_POSITION_MAX))
	{
		Command_Error_Number = 28;
		sprintf(Command_Error_String,"PIROT_Command_MOV: position %.2f out of range (0.0...%.2f).",
			position,COMMAND_MOV_POSITION_MAX);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 5;
		sprintf(Command_Error_String,"PIROT_Command_MOV: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_MOV: PI_MOV(usb_id=%d,axes=%s,position=%.2f).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,position);
#endif /* LOGGING */
	retval = PI_MOV(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,&position);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 9;
		sprintf(Command_Error_String,"PIROT_Command_MOV: PI_MOV failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 10;
		sprintf(Command_Error_String,"PIROT_Command_MOV: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_MOV: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Enable/Disable SerVO control of the rotator axis. Uses the PI rotator library "PI_SVO" routine / "SVO" command.
 * @param enable A boolean as an integer. IF TRUE, enable servo control of the rotator, otherwise disable it.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_IS_BOOLEAN
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_SVO(int enable)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_SVO(enable=%d): Started.",enable);
#endif /* LOGGING */
	if(!PIROT_IS_BOOLEAN(enable))
	{
		Command_Error_Number = 34;
		sprintf(Command_Error_String,"PIROT_Command_SVO: enable %d was not a boolean.",enable);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 35;
		sprintf(Command_Error_String,"PIROT_Command_SVO: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_SVO: PI_SVO(usb_id=%d,axes=%s,enable=%d).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,enable);
#endif /* LOGGING */
	retval = PI_SVO(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,&enable);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 36;
		sprintf(Command_Error_String,"PIROT_Command_SVO: PI_SVO failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 37;
		sprintf(Command_Error_String,"PIROT_Command_SVO: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_SVO: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Enable or disable the TRigger Output mode for the rotator. Uses the PI rotator library "PI_TRO" routine.
 * @param enable A boolean integer, TRUE to enable trigger output for the rotator, and FALSE to disable it.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_IS_BOOLEAN
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_TRO(int enable)
{
	int trigger_output_id_list[1];
	int trigger_state_list[1];
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_TRO(enable=%d): Started.",enable);
#endif /* LOGGING */
	if(!PIROT_IS_BOOLEAN(enable))
	{
		Command_Error_Number = 29;
		sprintf(Command_Error_String,"PIROT_Command_TRO: enable (%d) was not a boolean.",enable);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 25;
		sprintf(Command_Error_String,"PIROT_Command_TRO: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	trigger_output_id_list[0] = 1;
	trigger_state_list[0] = enable;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Command_TRO: PI_TRO(usb_id=%d,trigger_output_id_list[0]=%d,trigger_state_list[0]=%d).",
			 PIROT_USB_Get_ID(),trigger_output_id_list[0],trigger_state_list[0]);
#endif /* LOGGING */

	retval = PI_TRO(PIROT_USB_Get_ID(),trigger_output_id_list,trigger_state_list,1);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 26;
		sprintf(Command_Error_String,"PIROT_Command_TRO: PI_TRO failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 27;
		sprintf(Command_Error_String,"PIROT_Command_TRO: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_TRO: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the VELocitiy of the rotator axis. Uses the PI rotator library "PI_VEL" routine.
 * @param velocity The velocity of the rotator, in degrees per second, between 0 and COMMAND_ROTATOR_VELOCITY_MAX.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #COMMAND_ROTATOR_VELOCITY_MAX
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_VEL(double velocity)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_VEL(velocity=%.2f): Started.",velocity);
#endif /* LOGGING */
	if((velocity < 0.0) || (velocity > COMMAND_ROTATOR_VELOCITY_MAX))
	{
		Command_Error_Number = 30;
		sprintf(Command_Error_String,"PIROT_Command_VEL: velocity %.2f out of range (0.0...%.2f).",
			velocity,COMMAND_ROTATOR_VELOCITY_MAX);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 31;
		sprintf(Command_Error_String,"PIROT_Command_VEL: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_VEL: PI_VEL(usb_id=%d,axes=%s,velocity=%.2f).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,velocity);
#endif /* LOGGING */
	retval = PI_VEL(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,&velocity);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 32;
		sprintf(Command_Error_String,"PIROT_Command_VEL: PI_VEL failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 33;
		sprintf(Command_Error_String,"PIROT_Command_VEL: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_VEL: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Query the controller error state directly. We normally use PIROT_Command_Get_PI_Library_Error to get the error
 * (and also any associated library error), however this routine can be used at startup to effectively
 * reset the controller error state (which is what I believe "ERR?" does as a side effect).
 * Uses the PI rotator library "PI_qERR" routine, which issues a "ERR?" command to the controller.
 * @param error_number A pointer to an integer. On a successful call to this routine, the value
 *        of the integer pointed to, will contain the current controller error number.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_Query_ERR(int *error_number)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_ERR: Started.");
#endif /* LOGGING */
	if(error_number == NULL)
	{
		Command_Error_Number = 21;
		sprintf(Command_Error_String,"PIROT_Command_Query_ERR: error_number was NULL.");
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 22;
		sprintf(Command_Error_String,"PIROT_Command_Query_ERR: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_Query_ERR: PI_qERR(usb_id=%d,error_number=%p).",
			 PIROT_USB_Get_ID(),error_number);
#endif /* LOGGING */
	retval = PI_qERR(PIROT_USB_Get_ID(),error_number);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 23;
		sprintf(Command_Error_String,"PIROT_Command_Query_ERR: PI_qERR failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Command_Query_ERR: PI_qERR returned error_number %d,retval %d.",(*error_number),retval);
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 24;
		sprintf(Command_Error_String,"PIROT_Command_Query_ERR: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_ERR: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Query wether the rotator is on-target (has reached it's target position). 
 * Uses the PI rotator library "PI_qONT" routine, which issues a "ONT?" command to the controller.
 * @param on_target A pointer to an integer. On a successful call to this routine, the value
 *        of the integer pointed to, will contain TRUE if the rotator is on target, and FALSE if it hasn't reached the
 *        target position yet.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 * @see pirot_usb.html#PIROT_USB_Get_ID
 */
int PIROT_Command_Query_ONT(int *on_target)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_ONT: Started.");
#endif /* LOGGING */
	if(on_target == NULL)
	{
		Command_Error_Number = 11;
		sprintf(Command_Error_String,"PIROT_Command_Query_ONT: on_target was NULL.");
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 12;
		sprintf(Command_Error_String,"PIROT_Command_Query_ONT: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_Query_ONT: PI_qONT(usb_id=%d,axes=%s,on_target=%p).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,on_target);
#endif /* LOGGING */
	retval = PI_qONT(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,on_target);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 13;
		sprintf(Command_Error_String,"PIROT_Command_Query_ONT: PI_qONT failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Command_Query_ONT: PI_qONT returned on_target %d,retval %d.",(*on_target),retval);
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 14;
		sprintf(Command_Error_String,"PIROT_Command_Query_ONT: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_ONT: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Query the current position of the rotator. Uses the PI rotator library "PI_qPOS" routine, which issues a "POS?"
 * command to the controller.
 * @param position A pointer to a double. On a successful call to this routine, the value
 *        of the double pointed to, will contain the current rotator position.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_ROTATOR_AXIS
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_Query_POS(double *position)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_POS: Started.");
#endif /* LOGGING */
	if(position == NULL)
	{
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"PIROT_Command_Query_POS: position was NULL.");
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"PIROT_Command_Query_POS: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_Query_POS: PI_qPOS(usb_id=%d,axes=%s,position=%p).",
			 PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,position);
#endif /* LOGGING */
	retval = PI_qPOS(PIROT_USB_Get_ID(),COMMAND_ROTATOR_AXIS,position);
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"PIROT_Command_Query_POS: PI_qPOS failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
			 "PIROT_Command_Query_POS: PI_qPOS returned position %.2f,retval %d.",(*position),retval);
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"PIROT_Command_Query_POS: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"PIROT_Command_Query_POS: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Query the PI library for the current error state of the library, and error state of the underly controller.
 * We have turned off automatic error checking in PIROT_USB_Open (using PI_SetErrorCheck), so we should really
 * call PI_GetError after every PI library call to check whether the underlying controller has an error.
 * See C-867-U1-Software-Reference.pdf Section 3.3.1 (P10).
 * @param pi_error_num The address on an integer, to store the PI library error number retrieved using PI_GetError.
 * @param pi_error_string The address of a string to be filled with a textual version of the error, 
 *        using PI_TranslateError. The string should have allocated memeory of length pi_error_string_length.
 * @param pi_error_string_length The maximum allowed length of the string pi_error_string.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_usb.html#PIROT_USB_Open
 */
int PIROT_Command_Get_PI_Library_Error(int *pi_error_num,char *pi_error_string,int pi_error_string_length)
{
	int retval; 

	if(pi_error_num == NULL)
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,"PIROT_Command_Get_PI_Library_Error: pi_error_num was NULL.");
		return FALSE;
	}
	if(pi_error_string == NULL)
	{
		Command_Error_Number = 7;
		sprintf(Command_Error_String,"PIROT_Command_Get_PI_Library_Error: pi_error_string was NULL.");
		return FALSE;
	}
	(*pi_error_num) = PI_GetError(PIROT_USB_Get_ID());
	retval = PI_TranslateError((*pi_error_num),pi_error_string,STRING_LENGTH);
	if(retval == FALSE)
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,"PIROT_Command_Get_PI_Library_Error: PI_TranslateError returned FALSE "
			":- not enough room for the error message.");
		return FALSE;
	}
	return TRUE;
}

/**
 * Stop the motion of the rotator immediately. Sets the error code to 10 (PI_CNTR_STOP/Controller was stopped by command).
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #PIROT_Command_Get_PI_Library_Error
 * @see pirot_usb.html#PIROT_USB_Get_ID
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_general.html#PIROT_Mutex_Lock
 * @see pirot_general.html#PIROT_Mutex_Unlock
 */
int PIROT_Command_STP(void)
{
        int retval,pi_error_num;
	char pi_error_string[STRING_LENGTH];

#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_STP: Started.");
#endif /* LOGGING */
#ifdef MUTEXED
	if(!PIROT_Mutex_Lock())
	{
		Command_Error_Number = 18;
		sprintf(Command_Error_String,"PIROT_Command_STP: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"PIROT_Command_STP: PI_STP(usb_id=%d).",PIROT_USB_Get_ID());
#endif /* LOGGING */
	retval = PI_STP(PIROT_USB_Get_ID());
	if(retval != TRUE)
	{
#ifdef MUTEXED
		PIROT_Mutex_Unlock();
#endif /* MUTEXED */
		PIROT_Command_Get_PI_Library_Error(&pi_error_num,pi_error_string,STRING_LENGTH);
		Command_Error_Number = 19;
		sprintf(Command_Error_String,"PIROT_Command_STP: PI_STP failed (%d) : %s.",pi_error_num,
			pi_error_string);
		return FALSE;
	}
#ifdef MUTEXED
	if(!PIROT_Mutex_Unlock())
	{
		Command_Error_Number = 20;
		sprintf(Command_Error_String,"PIROT_Command_STP: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_TERSE,"PIROT_Command_STP: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int PIROT_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Command_Error(void)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s PIROT_Command:Error(%d) : %s\n",time_string,Command_Error_Number,Command_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Command_Error_String(char *error_string)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s PIROT_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
