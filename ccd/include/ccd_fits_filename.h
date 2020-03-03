/* ccd_fits_filename.h */

#ifndef CCD_FITS_FILENAME_H
#define CCD_FITS_FILENAME_H

/**
 * The number of cameras to generate FITS filenames for.
 */
#define CCD_FITS_FILENAME_CAMERA_COUNT      (2)
/**
 * Enum defining types of exposure to put in the exposure code part of a LT FITS filename.
 * <ul>
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_ARC
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_SKYFLAT
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_STANDARD
 * <li>CCD_FITS_FILENAME_EXPOSURE_TYPE_LAMPFLAT
 * </ul>
 */
enum CCD_FITS_FILENAME_EXPOSURE_TYPE
{
	CCD_FITS_FILENAME_EXPOSURE_TYPE_ARC=0,CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS,
	CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK,CCD_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE,
	CCD_FITS_FILENAME_EXPOSURE_TYPE_SKYFLAT,CCD_FITS_FILENAME_EXPOSURE_TYPE_STANDARD,
	CCD_FITS_FILENAME_EXPOSURE_TYPE_LAMPFLAT
};

/**
 * Macro to check whether the parameter is a valid exposure type.
 * @see #CCD_FITS_FILENAME_EXPOSURE_TYPE
 */
#define CCD_FITS_FILENAME_IS_EXPOSURE_TYPE(value)	(((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_ARC)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_BIAS)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_DARK)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_SKYFLAT)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_STANDARD)|| \
							 ((value) == CCD_FITS_FILENAME_EXPOSURE_TYPE_LAMPFLAT))
/**
 * Enum defining the pipeline processing flag to put in the pipeline flag part of a LT FITS filename.
 * <ul>
 * <li>CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED
 * <li>CCD_FITS_FILENAME_PIPELINE_FLAG_REALTIME
 * <li>CCD_FITS_FILENAME_PIPELINE_FLAG_OFFLINE
 * </ul>
 */
enum CCD_FITS_FILENAME_PIPELINE_FLAG
{
	CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED=0,
	CCD_FITS_FILENAME_PIPELINE_FLAG_REALTIME=1,
	CCD_FITS_FILENAME_PIPELINE_FLAG_OFFLINE=2
};

/**
 * Macro to check whether the parameter is a valid pipeline flag.
 * @see #CCD_FITS_FILENAME_PIPELINE_FLAG
 */
#define CCD_FITS_FILENAME_IS_PIPELINE_FLAG(value)	(((value) == CCD_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED)|| \
							 ((value) == CCD_FITS_FILENAME_PIPELINE_FLAG_REALTIME)|| \
							 ((value) == CCD_FITS_FILENAME_PIPELINE_FLAG_OFFLINE))

/**
 * Default instrument code, used as first character as LT FITS filename for camera index 0.
 */
#define CCD_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE0 ('1')
/**
 * Default instrument code, used as first character as LT FITS filename for camera index 1.
 */
#define CCD_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE1 ('2')

extern char CCD_Fits_Filename_Default_Instrument_Code_List[];
 
extern int CCD_Fits_Filename_Initialise(int camera_index,char instrument_code,char *data_dir);
extern int CCD_Fits_Filename_Next_Multrun(int camera_index);
extern int CCD_Fits_Filename_Next_Run(int camera_index);
extern int CCD_Fits_Filename_Next_Window(int camera_index);
extern int CCD_Fits_Filename_Get_Filename(int camera_index,enum CCD_FITS_FILENAME_EXPOSURE_TYPE type,
					  enum CCD_FITS_FILENAME_PIPELINE_FLAG pipeline_flag,
					  char *filename,int filename_length);
extern int CCD_Fits_Filename_List_Add(char *filename,char ***filename_list,int *filename_count);
extern int CCD_Fits_Filename_List_Free(char ***filename_list,int *filename_count);
extern int CCD_Fits_Filename_Multrun_Get(int camera_index);
extern int CCD_Fits_Filename_Run_Get(int camera_index);
extern int CCD_Fits_Filename_Window_Get(int camera_index);
extern int CCD_Fits_Filename_Lock(char *filename);
extern int CCD_Fits_Filename_UnLock(char *filename);
extern int CCD_Fits_Filename_Get_Error_Number(void);
extern void CCD_Fits_Filename_Error(void);
extern void CCD_Fits_Filename_Error_String(char *error_string);

#endif
