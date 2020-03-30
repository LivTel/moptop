/* pirot_setup.c
** PI Rotator setup routines
** $Header$
*/
/**
 * PI Rotator setup routines.
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "pirot_general.h"
#include "pirot_command.h"
#include "pirot_move.h"
#include "pirot_setup.h"

/**
 * The initial angle to start a run of exposures from in degrees.
 * The rotator is set to start triggering at 0 degrees, we therefore set the initial angle to be less than that,
 * so the rotator has time to speed up to the correct velocity before starting to generate triggers for the camera.
 * We currently only support postive (clockwise) velocities, if we supported negative (counter clockwise) velocities
 * this start position should then be positive.
 */
#define SETUP_ROTATOR_INITIAL_ANGLE (-22.0)

/**
 * The maximum position angle the rotator can move to in degrees.
 */
#define SETUP_ROTATOR_LIMIT_MAX     (36000.0)
/**
 * Length of time (in seconds) to wait for a rotator movement to complete in the setup code.
 */
#define SETUP_ROTATOR_TIMEOUT       (30)

/* structures */
/**
 * Structure holding local data pertinent to the setup module. This consists of:
 * <ul>
 * <li><b>Run_Velocity</b> The velocity to run the rotator at during run's, in degrees/second.
 * <li><b>Trigger_Step_Angle</b> The trigger step size angle in degrees.
 * </ul>
 */

struct Setup_Struct
{
	double Run_Velocity;
	double Trigger_Step_Angle;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Variable holding error code of last operation performed.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see #PIROT_ERROR_STRING_LENGTH
 */
static char Setup_Error_String[PIROT_ERROR_STRING_LENGTH] = "";
/**
 * The internal data used by setup routines.
 * Initialised to:
 * <ul>
 * <li><b>Run_Velocity</b>       = 45.0 deg/s
 * <li><b>Trigger_Step_Angle</b> = 22.5 degrees (16 triggers per rotation) (PIROT_SETUP_TRIGGER_STEP_ANGLE_16)
 * </ul>
 * @see #PIROT_SETUP_TRIGGER_STEP_ANGLE_16
 * @see #Setup_Struct
 */
static struct Setup_Struct Setup_Data = {45.0 , PIROT_SETUP_TRIGGER_STEP_ANGLE_16 };


/* =======================================
**  external functions 
** ======================================= */
/**
 * Set the rotator velocity to use for the run (as opposed to initialisation). This routine should be called
 * before PIROT_Setup_Rotator which actually configures the controller. This routine just stores the velocity value
 * in Setup_Data.Run_Velocity.
 * @param velocity The velocity to use for the run, as a double, in degrees/s.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see pirot_general.html#PIROT_Log_Format
 */
int PIROT_Setup_Rotator_Run_Velocity(double velocity)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Rotator_Run_Velocity: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Rotator_Run_Velocity: Set velocity to %.2f.",velocity);
#endif /* LOGGING */
	Setup_Data.Run_Velocity = velocity;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Rotator_Run_Velocity: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the trigger step angle. This routine should be called
 * before PIROT_Setup_Rotator which actually configures the controller. This routine just stores the angle
 * in Setup_Data.Trigger_Step_Angle.
 * @param trigger_step_angle The trigger step angle, as a double, in degrees/s. Sensible values are:
 *        PIROT_SETUP_TRIGGER_STEP_ANGLE_8, PIROT_SETUP_TRIGGER_STEP_ANGLE_16, PIROT_SETUP_TRIGGER_STEP_ANGLE_32.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #PIROT_SETUP_TRIGGER_STEP_ANGLE_8
 * @see #PIROT_SETUP_TRIGGER_STEP_ANGLE_16
 * @see #PIROT_SETUP_TRIGGER_STEP_ANGLE_32
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see pirot_general.html#PIROT_Log_Format
 */
int PIROT_Setup_Trigger_Step_Angle(double trigger_step_angle)
{
	Setup_Error_Number = 0;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Trigger_Step_Angle: Started.");
#endif /* LOGGING */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Trigger_Step_Angle: Set angle to %.2f degrees.",
			 trigger_step_angle);
#endif /* LOGGING */
	Setup_Data.Trigger_Step_Angle = trigger_step_angle;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Trigger_Step_Angle: Finished.");
#endif /* LOGGING */
}

/**
 * Setup the PI rotator.
 * <ul>
 * <li>Stop any current movement of the rotator using PIROT_Command_STP / "STP".
 * <li>Clear any errors currently outstanding on the controller using the PIROT_Command_Query_ERR / "ERR?" command. 
 *     These are controller errors and not internal library errors.
 * <li>Disable rotator triggering using the PIROT_Command_TRO / "TRO 1 0" command.
 * <li>Set the initial rotator velocity to 360 deg/s using the PIROT_Command_VEL / "VEL 1 360.0" command.
 * <li>Enable servoing using the PIROT_Command_SVO / "SVO 1 1" command.
 * <li>Move to a reference point using the PIROT_Command_FRF / "FRF 1" command.
 * <li>Wait until the rotator is on target (for up to SETUP_ROTATOR_TIMEOUT s), using PIROT_Move_Wait_For_On_Target.
 * <li>Set the rotator velocity for the run using the PIROT_Command_VEL / "VEL" command, 
 *     using the previously configured value in Setup_Data.Run_Velocity.
 * <li>We configure trigger output 1 (physical pin 5) on axis 1 (the rotator), 
 *     using a PIROT_Command_CTO / "CTO 1 2 1" command.
 * <li>We configure the trigger polarity high, using a PIROT_Command_CTO / "CTO 1 7 1" command.
 * <li>We configure the trigger step size angle using a PIROT_Command_CTO / "CTO 1 1 <n>" command. The step size
 *     is retrieved from the previously configured value in Setup_Data.Trigger_Step_Angle.
 * <li>Set the trigger mode to position plus offset using a PIROT_Command_CTO command with 
 *     CTO_PARAMETER_TRIGGER_MODE parameter and CTO_TRIGGER_MODE_POSITION_PLUS_OFFSET value ("CTO 1 3 7").
 * <li>We set the first trigger position to 0 degrees, using a PIROT_Command_CTO command with 
 *     CTO_PARAMETER_TRIGGER_POSITION parameter ("CTO 1 10 0").
 * <li>We set the trigger begin position to 0 degrees, using a PIROT_Command_CTO command with 
 *     CTO_PARAMETER_START_THRESHOLD parameter ("CTO 1 8 0").
 * <li>We set the trigger end position to SETUP_ROTATOR_LIMIT_MAX degress, using PIROT_Command_CTO command with 
 *     CTO_PARAMETER_STOP_THRESHOLD parameter ("CTO 1 9 36000").
 * <li>We use PIROT_Command_MOV to start moving the rotator to it's initial position SETUP_ROTATOR_INITIAL_ANGLE.
 * <li>We wait until the rotator is on target (for up to SETUP_ROTATOR_TIMEOUT s), using PIROT_Move_Wait_For_On_Target.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #SETUP_ROTATOR_INITIAL_ANGLE
 * @see #SETUP_ROTATOR_LIMIT_MAX
 * @see #SETUP_ROTATOR_TIMEOUT
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see pirot_command.html#PIROT_COMMAND_CTO_PARAMETER_ENUM
 * @see pirot_command.html#PIROT_COMMAND_CTO_TRIGGER_MODE_ENUM
 * @see pirot_command.html#PIROT_Command_CTO
 * @see pirot_command.html#PIROT_Command_FRF
 * @see pirot_command.html#PIROT_Command_MOV
 * @see pirot_command.html#PIROT_Command_STP
 * @see pirot_command.html#PIROT_Command_SVO
 * @see pirot_command.html#PIROT_Command_TRO
 * @see pirot_command.html#PIROT_Command_VEL
 * @see pirot_command.html#PIROT_Command_Query_ERR
 * @see pirot_general.html#PIROT_GENERAL_ONE_SECOND_MS
 * @see pirot_general.html#PIROT_Log_Format
 * @see pirot_move.html#PIROT_Move_Wait_For_On_Target
 */
int PIROT_Setup_Rotator(void)
{
	int error_number;

	Setup_Error_Number = 0;
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Rotator: Started.");
#endif /* LOGGING */
	/* stop the rotator moving */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Stop any rotator movement.");
#endif /* LOGGING */
	if(!PIROT_Command_STP())
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to stop any rotator movement.");
		return FALSE;
	}
	/* clear errors */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Clear any controller errors.");
#endif /* LOGGING */
	if(!PIROT_Command_Query_ERR(&error_number))
	{
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to clear any controller errors.");
		return FALSE;
	}
	/* disable triggering */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Disable triggering.");
#endif /* LOGGING */
	if(!PIROT_Command_TRO(FALSE))
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to disable triggering.");
		return FALSE;
	}
	/* set initial velocity */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Set initial velocity to 360 deg/s.");
#endif /* LOGGING */
	if(!PIROT_Command_VEL(360.0))
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to set initial velocity.");
		return FALSE;
	}
	/* enable servoing */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Enable servoing.");
#endif /* LOGGING */
	if(!PIROT_Command_SVO(TRUE))
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to enable servoing.");
		return FALSE;
	}
	/* move to a reference point */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Move to a reference point.");
#endif /* LOGGING */
	if(!PIROT_Command_FRF())
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to move to a reference point.");
		return FALSE;
	}
	/* wait until the rotator is on target */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Wait until the rotator is on target.");
#endif /* LOGGING */
	if(!PIROT_Move_Wait_For_On_Target(SETUP_ROTATOR_TIMEOUT * PIROT_GENERAL_ONE_SECOND_MS))
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Rotator failed to move on target.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Set the run velocity to %.2f deg/s.",
			 Setup_Data.Run_Velocity);
#endif /* LOGGING */
	if(!PIROT_Command_VEL(Setup_Data.Run_Velocity))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to set run velocity to %.2f deg/s.",
			Setup_Data.Run_Velocity);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,
			 "PIROT_Setup_Rotator: Configure trigger output 1 (physical pin 5) on axis 1 (the rotator).");
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_AXIS,1))
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to configure trigger output 1 (physical pin 5) "
			"on axis 1 (the rotator).");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Configure trigger polarity high.");
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_POLARITY,1))
	{
		Setup_Error_Number = 10;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Failed to configure trigger polarity high.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Configure the trigger step angle to %.2f degrees.",
			 Setup_Data.Trigger_Step_Angle);
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_TRIGGER_STEP,Setup_Data.Trigger_Step_Angle))
	{
		Setup_Error_Number = 11;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to configure trigger step angle to %.2f degrees.",
			Setup_Data.Trigger_Step_Angle);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Set the trigger mode to position plus offset.");
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_TRIGGER_MODE,CTO_TRIGGER_MODE_POSITION_PLUS_OFFSET))
	{
		Setup_Error_Number = 12;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to configure trigger mode to position plus offset.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Set the first trigger position to 0 degrees.");
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_TRIGGER_POSITION,0.0))
	{
		Setup_Error_Number = 13;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to set the first trigger position to 0 degrees.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Set the trigger begin position to 0 degrees.");
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_START_THRESHOLD,0.0))
	{
		Setup_Error_Number = 14;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to set the trigger begin position to 0 degrees.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,
			 "PIROT_Setup_Rotator: Set the trigger end position to %.2f degrees.",SETUP_ROTATOR_LIMIT_MAX);
#endif /* LOGGING */
	if(!PIROT_Command_CTO(CTO_PARAMETER_STOP_THRESHOLD,SETUP_ROTATOR_LIMIT_MAX))
	{
		Setup_Error_Number = 15;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to set the trigger end position to %.2f degrees.",
			SETUP_ROTATOR_LIMIT_MAX);
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Start Move to intial rotator angle %.2f.",
			 SETUP_ROTATOR_INITIAL_ANGLE);
#endif /* LOGGING */
	if(!PIROT_Command_MOV(SETUP_ROTATOR_INITIAL_ANGLE))
	{
		Setup_Error_Number = 16;
		sprintf(Setup_Error_String,
			"PIROT_Setup_Rotator: Failed to start to move the rotator to it's initial angle %.2f.",
			SETUP_ROTATOR_INITIAL_ANGLE);
		return FALSE;
	}
	/* wait until the rotator is on target */
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERBOSE,"PIROT_Setup_Rotator: Wait until the rotator is on target.");
#endif /* LOGGING */
	if(!PIROT_Move_Wait_For_On_Target(SETUP_ROTATOR_TIMEOUT * PIROT_GENERAL_ONE_SECOND_MS))
	{
		Setup_Error_Number = 17;
		sprintf(Setup_Error_String,"PIROT_Setup_Rotator: Rotator failed to move on target.");
		return FALSE;
	}
#if LOGGING > 0
	PIROT_Log_Format(LOG_VERBOSITY_VERY_TERSE,"PIROT_Setup_Rotator: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Return the last configured rotator run velocity.
 * @return The last configured rotator run velocity in degrees/s, stored in Setup_Data.Run_Velocity.
 * @see #Setup_Data
 */
double PIROT_Setup_Rotator_Run_Velocity_Get(void)
{
	return Setup_Data.Run_Velocity;
}

/**
 * Return the last configured trigger step angle.
 * @return The last configured trigger step angle in degrees, stored in Setup_Data.Trigger_Step_Angle.
 * @see #Setup_Data
 */
double PIROT_Setup_Trigger_Step_Angle_Get(void)
{
	return Setup_Data.Trigger_Step_Angle;
}

/**
 * Check that the rotator is 'in position' (i.e. on target) at the right start position.
 * @return The routine returns true if the rotator is 'on target' and at or very near the rotator's start angle
 *         (SETUP_ROTATOR_INITIAL_ANGLE). The routine returns FALSE if this is not the case, or there is a fault
 *         communicating with the rotator.
 * @see #PIROT_SETUP_ROTATOR_TOLERANCE
 * @see #SETUP_ROTATOR_INITIAL_ANGLE
 * @see pirot_command.html#PIROT_Command_Query_ONT
 * @see pirot_command.html#PIROT_Command_Query_POS
 */
int PIROT_Setup_Is_Rotator_At_Start_Position(void)
{
	double current_position;
	int on_target;
	
	/* check we are on target i.e. not moving */
	if(!PIROT_Command_Query_ONT(&on_target))
	{
		Setup_Error_Number = 18;
		sprintf(Setup_Error_String,"PIROT_Setup_Is_Rotator_At_Start_Position: PIROT_Command_Query_ONT failed.");
		return FALSE;
	}
	/* if we are moving and not on target return false */
	if(on_target == FALSE)
	{
		Setup_Error_Number = 19;
		sprintf(Setup_Error_String,"PIROT_Setup_Is_Rotator_At_Start_Position: We are not on target.");
		return FALSE;		
	}
	/* query position */
	if(!PIROT_Command_Query_POS(&current_position))
	{
		Setup_Error_Number = 20;
		sprintf(Setup_Error_String,"PIROT_Setup_Is_Rotator_At_Start_Position: PIROT_Command_Query_POS failed.");
		return FALSE;
	}
	/* are we very close to the correct initial angle */
	if(fabs(current_position-SETUP_ROTATOR_INITIAL_ANGLE) > PIROT_SETUP_ROTATOR_TOLERANCE)
	{
		Setup_Error_Number = 21;
		sprintf(Setup_Error_String,"PIROT_Setup_Is_Rotator_At_Start_Position: "
			"We are in the wrong start position (%.2f degs vs %.2f degs).",
			current_position,SETUP_ROTATOR_INITIAL_ANGLE);
		return FALSE;
	}
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Setup_Error_Number
 */
int PIROT_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Setup_Error(void)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s PIROT_Setup:Error(%d) : %s\n",time_string,Setup_Error_Number,Setup_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see pirot_general.html#PIROT_General_Get_Current_Time_String
 */
void PIROT_Setup_Error_String(char *error_string)
{
	char time_string[32];

	PIROT_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s PIROT_Setup:Error(%d) : %s\n",time_string,
		Setup_Error_Number,Setup_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
