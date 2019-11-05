// MoptopConstants.java
// $Header$
package ngat.moptop;

import java.lang.*;
import java.io.*;

/**
 * This class holds some constant values for the Moptop program. 
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class MoptopConstants
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Error code. No error.
	 */
	public final static int MOPTOP_ERROR_CODE_NO_ERROR 			= 0;
	/**
	 * The base Error number, for all Moptop error codes. 
	 * See http://ltdevsrv.livjm.ac.uk/~dev/errorcodes.html for details.
	 */
	public final static int MOPTOP_ERROR_CODE_BASE 			= 1800000;
	/**
	 * Default thread priority level. This is for the server thread. Currently this has the highest priority,
	 * so that new connections are always immediately accepted.
	 * This number is the default for the <b>moptop.thread.priority.server</b> property, if it does not exist.
	 */
	public final static int MOPTOP_DEFAULT_THREAD_PRIORITY_SERVER		= Thread.NORM_PRIORITY+2;
	/**
	 * Default thread priority level. 
	 * This is for server connection threads dealing with sub-classes of the INTERRUPT
	 * class. Currently these have a higher priority than other server connection threads,
	 * so that INTERRUPT commands are always responded to even when another command is being dealt with.
	 * This number is the default for the <b>moptop.thread.priority.interrupt</b> property, if it does not exist.
	 */
	public final static int MOPTOP_DEFAULT_THREAD_PRIORITY_INTERRUPT	        = Thread.NORM_PRIORITY+1;
	/**
	 * Default thread priority level. This is for most server connection threads. 
	 * Currently this has a normal priority.
	 * This number is the default for the <b>moptop.thread.priority.normal</b> property, if it does not exist.
	 */
	public final static int MOPTOP_DEFAULT_THREAD_PRIORITY_NORMAL		= Thread.NORM_PRIORITY;
	/**
	 * Default thread priority level. This is for the Telescope Image Transfer server/client threads. 
	 * Currently this has the lowest priority, so that the camera control is not interrupted by image
	 * transfer requests.
	 * This number is the default for the <b>moptop.thread.priority.tit</b> property, if it does not exist.
	 */
	public final static int MOPTOP_DEFAULT_THREAD_PRIORITY_TIT		= Thread.MIN_PRIORITY;
	/**
	 * The maximum number of C layers this software can talk to.
	 */
	public final static int MOPTOP_MAX_C_LAYER_COUNT                        = 2;
}
