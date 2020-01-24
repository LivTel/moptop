/* pirot_command.h
** $Header$
*/

#ifndef PIROT_COMMAND_H
#define PIROT_COMMAND_H

/**
 * Enum describing the allowed trigger_parameter's for the CTO command, as described in the rotators user manual:
 * (C-867-1U-UserManual-MS223E200.pdf, P171).
 * <ul>
 * <li>CTO_PARAMETER_TRIGGER_STEP
 * <li>CTO_PARAMETER_AXIS
 * <li>CTO_PARAMETER_TRIGGER_MODE
 * <li>CTO_PARAMETER_POLARITY
 * <li>CTO_PARAMETER_START_THRESHOLD
 * <li>CTO_PARAMETER_STOP_THRESHOLD
 * <li>CTO_PARAMETER_TRIGGER_POSITION
 * <li>CTO_PARAMETER_PULSE_WIDTH
 * </ul>
 */
enum PIROT_COMMAND_CTO_PARAMETER_ENUM
{
	CTO_PARAMETER_TRIGGER_STEP=1,
	CTO_PARAMETER_AXIS=2,
	CTO_PARAMETER_TRIGGER_MODE=3,
	CTO_PARAMETER_POLARITY=7,
	CTO_PARAMETER_START_THRESHOLD=8,
	CTO_PARAMETER_STOP_THRESHOLD=9,
	CTO_PARAMETER_TRIGGER_POSITION=10,
	CTO_PARAMETER_PULSE_WIDTH=11
};

/**
 * Valid numbers to pass to CTO, as a value for the trigger mode parameter (CTO_PARAMETER_TRIGGER_MODE):
 * <ul>
 * <li>CTO_TRIGGER_MODE_POSITION_DISTANCE
 * <li>CTO_TRIGGER_MODE_ON_TARGET
 * <li>CTO_TRIGGER_MODE_MOTION_ERROR
 * <li>CTO_TRIGGER_MODE_IN_MOTION
 * <li>CTO_TRIGGER_MODE_POSITION_PLUS_OFFSET
 * <li>CTO_TRIGGER_MODE_SINGLE_POSITION
 * <li>CTO_TRIGGER_MODE_HARDWARE_TRIGGER
 * </ul>
 * See the user manual C-867-1U-UserManual-MS223E200.pdf,P171/P163.
 * @see #PIROT_COMMAND_CTO_PARAMETER_ENUM
 */
enum PIROT_COMMAND_CTO_TRIGGER_MODE_ENUM
{
	CTO_TRIGGER_MODE_POSITION_DISTANCE=0,
	CTO_TRIGGER_MODE_ON_TARGET=2,
	CTO_TRIGGER_MODE_MOTION_ERROR=5,
	CTO_TRIGGER_MODE_IN_MOTION=6,
	CTO_TRIGGER_MODE_POSITION_PLUS_OFFSET=7,
	CTO_TRIGGER_MODE_SINGLE_POSITION=8,
	CTO_TRIGGER_MODE_HARDWARE_TRIGGER=9
};

/**
 * Hash define to determine whether the parameter is a valid CTO trigger parameter number.
 * @param p The parameter number to test.
 * @see #PIROT_COMMAND_CTO_PARAMETER_ENUM
 */
#define PIROT_COMMAND_IS_CTO_PARAMETER(p) (((p)==CTO_PARAMETER_TRIGGER_STEP)||((p)==CTO_PARAMETER_AXIS)|| \
	((p)==CTO_PARAMETER_TRIGGER_MODE)||((p)==CTO_PARAMETER_POLARITY)||((p)==CTO_PARAMETER_START_THRESHOLD)|| \
	((p)==CTO_PARAMETER_STOP_THRESHOLD)||((p)==CTO_PARAMETER_TRIGGER_POSITION)||((p)==CTO_PARAMETER_PULSE_WIDTH))

extern int PIROT_Command(char *command_string);
extern int PIROT_Command_CTO(enum PIROT_COMMAND_CTO_PARAMETER_ENUM trigger_parameter,double value);
extern int PIROT_Command_FRF(void);
extern int PIROT_Command_MOV(double position);
extern int PIROT_Command_SVO(int enable);
extern int PIROT_Command_TRO(int enable);
extern int PIROT_Command_VEL(double velocity);
extern int PIROT_Command_Query_ERR(int *error_number);
extern int PIROT_Command_Query_ONT(int *on_target);
extern int PIROT_Command_Query_POS(double *position);
extern int PIROT_Command_Get_PI_Library_Error(int *pi_error_num,char *pi_error_string,int pi_error_string_length);
extern int PIROT_Command_Get_Error_Number(void);
extern int PIROT_Command_STP(void);
extern void PIROT_Command_Error(void);
extern void PIROT_Command_Error_String(char *error_string);

/*
** $Log$
*/

#endif
