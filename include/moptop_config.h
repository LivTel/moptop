/* moptop_config.h
*/
#ifndef MOPTOP_CONFIG_H
#define MOPTOP_CONFIG_H

extern int Moptop_Config_Load(char *filename);
extern int Moptop_Config_Shutdown(void);
extern int Moptop_Config_Get_String(char *key, char **value);
extern int Moptop_Config_Get_Character(char *key, char *value);
extern int Moptop_Config_Get_Integer(char *key, int *i);
extern int Moptop_Config_Get_Long(char *key, long *l);
extern int Moptop_Config_Get_Unsigned_Short(char *key,unsigned short *us);
extern int Moptop_Config_Get_Double(char *key, double *d);
extern int Moptop_Config_Get_Float(char *key, float *f);
extern int Moptop_Config_Get_Boolean(char *key, int *boolean);
#endif
