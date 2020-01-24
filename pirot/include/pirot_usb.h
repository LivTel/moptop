/* pirot_usb.h
** $Header$
*/
#ifndef PIROT_USB_H
#define PIROT_USB_H
/**
 * The baud rate to talk to the PI rotator at.
 */
#define PIROT_USB_BAUD_RATE        (460800)

extern int PIROT_USB_Open(char *device_name,int baud_rate);
extern int PIROT_USB_Close(void);
extern int PIROT_USB_Get_ID(void);
extern int PIROT_USB_Get_Error_Number(void);
extern void PIROT_USB_Error(void);
extern void PIROT_USB_Error_String(char *error_string);
/*
** $Log$
*/
#endif
