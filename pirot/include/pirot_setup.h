/* pirot_setup.h
** $Header$
*/

#ifndef PIROT_SETUP_H
#define PIROT_SETUP_H

/**
 * Trigger step angle to use to produce 8 triggers per revolution of the rotator.
 * Used as the trigger_step_angle parameter for the PIROT_Setup_Trigger_Step_Angle routine.
 * @see #PIROT_Setup_Trigger_Step_Angle
 */
#define PIROT_SETUP_TRIGGER_STEP_ANGLE_8      (45.0)
/**
 * Trigger step angle to use to produce 16 triggers per revolution of the rotator.
 * Used as the trigger_step_angle parameter for the PIROT_Setup_Trigger_Step_Angle routine.
 * @see #PIROT_Setup_Trigger_Step_Angle
 */
#define PIROT_SETUP_TRIGGER_STEP_ANGLE_16     (22.5)
/**
 * Trigger step angle to use to produce 32 triggers per revolution of the rotator.
 * Used as the trigger_step_angle parameter for the PIROT_Setup_Trigger_Step_Angle routine.
 * @see #PIROT_Setup_Trigger_Step_Angle
 */
#define PIROT_SETUP_TRIGGER_STEP_ANGLE_32     (11.25)
/**
 * Default rotator tolerance in degrees.
 */
#define PIROT_SETUP_ROTATOR_TOLERANCE         (0.004)

extern int PIROT_Setup_Rotator_Run_Velocity(double velocity);
extern int PIROT_Setup_Trigger_Step_Angle(double trigger_step_angle);
extern int PIROT_Setup_Rotator(void);
extern double PIROT_Setup_Rotator_Run_Velocity_Get(void);
extern double PIROT_Setup_Trigger_Step_Angle_Get(void);
extern int PIROT_Setup_Is_Rotator_At_Start_Position(void);
extern int PIROT_Setup_Get_Error_Number(void);
extern void PIROT_Setup_Error(void);
extern void PIROT_Setup_Error_String(char *error_string);

/*
** $Log$
*/

#endif
