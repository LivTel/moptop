/* pirot_move.h
** $Header$
*/

#ifndef PIROT_MOVE_H
#define PIROT_MOVE_H
extern int PIROT_Move_Wait_For_On_Target(int timeout_ms);
extern int PIROT_Move_Get_Error_Number(void);
extern void PIROT_Move_Error(void);
extern void PIROT_Move_Error_String(char *error_string);

/*
** $Log$
*/

#endif
