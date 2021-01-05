/* moptop_multrun.h */
#ifndef MOPTOP_MULTRUN_H
#define MOPTOP_MULTRUN_H
#include <time.h> /* struct timespec */

extern int Moptop_Multrun_Exposure_Length_Set(double exposure_length_s);
extern int Moptop_Multrun_Filter_Name_Set(char *filter_name);
extern int Moptop_Multrun_Flip_Set(int flip_x,int flip_y);
extern int Moptop_Multrun_Setup(int *multrun_number);
extern int Moptop_Multrun(int exposure_length_ms,int use_exposure_length,int exposure_count,int use_exposure_count,
			  int do_standard,char ***filename_list,int *filename_count);
extern int Moptop_Multrun_Abort(void);

/* status routines */
extern int Moptop_Multrun_In_Progress(void);
extern int Moptop_Multrun_Count_Get(void);
extern int Moptop_Multrun_Per_Frame_Exposure_Length_Get(void);
extern int Moptop_Multrun_Exposure_Start_Time_Get(struct timespec *exposure_start_time);
extern int Moptop_Multrun_Exposure_Index_Get(void);
extern int Moptop_Multrun_Multrun_Get(void);
extern int Moptop_Multrun_Run_Get(void);
extern int Moptop_Multrun_Window_Get(void);

/* rotator caching setters/getters */
extern int Moptop_Multrun_Rotator_Speed_Set(char *rotator_speed);
extern void Moptop_Multrun_Rotator_Run_Velocity_Set(double velocity);
extern void Moptop_Multrun_Rotator_Step_Angle_Set(double step_angle);
extern void Moptop_Multrun_Rotator_Speed_Get(char *rotator_speed);
extern double Moptop_Multrun_Rotator_Run_Velocity_Get(void);
extern double Moptop_Multrun_Rotator_Step_Angle_Get(void);

/* helper routines for saving FITS images */
extern void Moptop_Multrun_Flip_X(int ncols,int nrows,unsigned short *exposure_data);
extern void Moptop_Multrun_Flip_Y(int ncols,int nrows,unsigned short *exposure_data);


#endif
