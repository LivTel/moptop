// MULTDARKImplementation.java
// $Id$
package ngat.moptop;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.phase2.*;
import ngat.moptop.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the MULTDARK command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.moptop.HardwareImplementation
 */
public class MULTDARKImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public MULTDARKImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTDARK&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTDARK";
	}

	/**
	 * This method returns the MULTDARK command's acknowledge time. 
	 * <ul>
	 * <li>The acknowledge time is the exposure length plus a nominal readout time (1s) 
	 *     multiplied by the exposure count.
	 * <li>The default acknowledge time is added to the total and returned.
	 * </ul>
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTDARK#getExposureTime
	 * @see MULTDARK#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTDARK multDarkCommand = (MULTDARK)command;
		ACK acknowledge = null;
		int exposureLength,exposureCount,ackTime=0;

		exposureLength = multDarkCommand.getExposureTime();
		exposureCount = multDarkCommand.getNumberExposures();
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount);
		ackTime = (exposureLength+1000)*exposureCount;
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTDARK command. 
	 * <ul>
	 * <li>It moves the fold mirror to a dark location.
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). A filtered subset
	 *     is sent on to the C layer.
	 * <li>A thread of class MultdarkCommandThread, is instantiated, and started, for each C Layer.
	 *     Each thread sends the multdark command to it's C layer, and then waits for an answer, 
	 *     and parses the result.
	 * <li>We enter a loop monitoring the threads, until they have all terminated.
	 * <li>The done object is setup. We check whether any of the C layer threads threw an exception, 
	 *     during execution.
	 * </ul>
	 * @see #testAbort
	 * @see ngat.moptop.HardwareImplementation#moveFoldToDark
	 * @see ngat.moptop.HardwareImplementation#clearFitsHeaders
	 * @see ngat.moptop.HardwareImplementation#setFitsHeaders
	 * @see ngat.moptop.HardwareImplementation#getFitsHeadersFromISS
	 * @see ngat.moptop.MoptopConstants#MOPTOP_MAX_C_LAYER_COUNT
	 * @see MultdarkCommandThread
	 * @see MultdarkCommandThread#setMoptop
	 * @see MultdarkCommandThread#setStatus
	 * @see MultdarkCommandThread#setCLayerIndex
	 * @see MultdarkCommandThread#setMultdarkCommandParameters
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTDARK multDarkCommand = (MULTDARK)command;
		MULTDARK_DONE multDarkDone = new MULTDARK_DONE(command.getId());
		MultdarkCommandThread multdarkThreadList[] =
			new MultdarkCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int exposureLength,exposureCount,cLayerCount,threadFinishedCount;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multDarkCommand,multDarkDone) == true)
			return multDarkDone;
	// move the fold mirror to a dark location
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFoldToDark(multDarkCommand,multDarkDone) == false)
			return multDarkDone;
		if(testAbort(multDarkCommand,multDarkDone) == true)
			return multDarkDone;
		// get multdark data
		exposureLength = multDarkCommand.getExposureTime();
		exposureCount = multDarkCommand.getNumberExposures();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount+".");
		// get fits headers
		clearFitsHeaders();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multDarkCommand,multDarkDone) == false)
			return multDarkDone;
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multDarkCommand,multDarkDone) == false)
			return multDarkDone;
		if(testAbort(multDarkCommand,multDarkDone) == true)
			return multDarkDone;
		// call multdark command
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting MULTDARK C layer command threads.");
		try
		{
			// start multdark command threads - one per c layer
			cLayerCount = status.getPropertyInteger("moptop.c.count");
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{
				multdarkThreadList[cLayerIndex] = new MultdarkCommandThread();
				multdarkThreadList[cLayerIndex].setMoptop(moptop);
				multdarkThreadList[cLayerIndex].setStatus(status);
				multdarkThreadList[cLayerIndex].setCLayerIndex(cLayerIndex);
				multdarkThreadList[cLayerIndex].setMultdarkCommandParameters(exposureLength,
											     exposureCount);
				moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":processCommand:Starting MULTDARK C layer command thread "+cLayerIndex+".");
				multdarkThreadList[cLayerIndex].start();
			}
			// wait for all multdark command threads to terminate
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:Waiting for MULTDARK C layer command threads to terminate.");
			do
			{
				threadFinishedCount = 0;
				for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
				{
					if(multdarkThreadList[cLayerIndex].isAlive())
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTDARK C layer command thread "+
							   cLayerIndex+" still running.");
					}
					else
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTDARK C layer command thread "+
							   cLayerIndex+" has finished.");
						threadFinishedCount++;
					}
					// sleep a bit
					Thread.sleep(1000);
				}// end for
			} while(threadFinishedCount < cLayerCount);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:All MULTDARK C layer command threads have finished.");
		}
		catch(Exception e )
		{
			moptop.error(this.getClass().getName()+":processCommand:Multdark command thread failed:",e);
			multDarkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2700);
			multDarkDone.setErrorString(this.getClass().getName()+
						    ":processCommand:sendMultdarkCommand failed:"+e);
			multDarkDone.setSuccessful(false);
			return multDarkDone;
		}
		// setup return values.
		// setup multdark done
		// standard success values
		multDarkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		multDarkDone.setErrorString("");
		multDarkDone.setSuccessful(true);
		// check to see whether any of the C layer threw an exception.
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multdarkThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":processCommand:C Layer "+cLayerIndex+
					     " returned exception:",multdarkThreadList[cLayerIndex].getException());
				multDarkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2701);
				multDarkDone.setErrorString(this.getClass().getName()+
							    ":processCommand:MULTDARK C Layer "+cLayerIndex+
							    " returned exception:"+
							    multdarkThreadList[cLayerIndex].getException());
				multDarkDone.setSuccessful(false);
			}
			else
			{
				// this c Layer multdark was successful
				// If this is C layer 0 (which is on the same machine as the Java layer),
				// pull last filename. This means the filename returned to the IcsGUI can be
				// displayed by it
				if(cLayerIndex == 0)
					multDarkDone.setFilename(multdarkThreadList[cLayerIndex].getLastFilename());
			}
		}// end for on cLayerIndex
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multDarkDone;
	}
}
