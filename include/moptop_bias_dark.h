/* moptop_bias_dark.h */
#ifndef MOPTOP_BIAS_DARK_H
#define MOPTOP_BIAS_DARK_H

extern int Moptop_Bias_Dark_MultBias(int exposure_count,char ***filename_list,int *filename_count);
extern int Moptop_Bias_Dark_MultDark(int exposure_length_ms,int exposure_count,
				     char ***filename_list,int *filename_count);
extern int Moptop_Bias_Dark_Abort(void);
extern int Moptop_Bias_Dark_Flip_Set(int flip_x,int flip_y);

/* status routines */
extern int Moptop_Bias_Dark_In_Progress(void);
extern int Moptop_Bias_Dark_Count_Get(void);
extern int Moptop_Bias_Dark_Per_Frame_Exposure_Length_Get(void);
extern int Moptop_Bias_Dark_Multrun_Get(void);
extern int Moptop_Bias_Dark_Run_Get(void);

#endif
