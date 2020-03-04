/* moptop_command.h */
#ifndef MOPTOP_COMMAND_H
#define MOPTOP_COMMAND_H

extern int Moptop_Command_Abort(char *command_string,char **reply_string);
extern int Moptop_Command_Config(char *command_string,char **reply_string);
extern int Moptop_Command_Fits_Header(char *command_string,char **reply_string);
extern int Moptop_Command_Multrun(char *command_string,char **reply_string);
/*extern int Moptop_Command_Mult_Dark(char *command_string,char **reply_string);*/
extern int Moptop_Command_Status(char *command_string,char **reply_string);
extern int Moptop_Command_Temperature(char *command_string,char **reply_string);

#endif
