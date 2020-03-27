/* moptop_multrun.h */
#ifndef MOPTOP_MULTRUN_H
#define MOPTOP_MULTRUN_H
extern int Moptop_Multrun(int exposure_length_ms,int use_exposure_length,int exposure_count,int use_exposure_count,
			  char ***filename_list,int *filename_count);
extern int Moptop_Multrun_Abort(void);
extern int Moptop_Multrun_In_Progress(void);
extern void Moptop_Multrun_Rotator_Run_Velocity_Set(double velocity);
extern void Moptop_Multrun_Rotator_Step_Angle_Set(double step_angle);
extern double Moptop_Multrun_Rotator_Run_Velocity_Get(void);
extern double Moptop_Multrun_Rotator_Step_Angle_Get(void);

#endif
