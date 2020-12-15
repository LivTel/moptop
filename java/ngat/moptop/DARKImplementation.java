// DARKImplementation.java
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
 * This class provides the implementation for the DARK command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.moptop.HardwareImplementation
 */
public class DARKImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public DARKImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.DARK&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.DARK";
	}

	/**
	 * This method returns the DARK command's acknowledge time. 
	 * <ul>
	 * <li>The acknowledge time is the exposure length plus a nominal readout time (1s).
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
		DARK darkCommand = (DARK)command;
		ACK acknowledge = null;
		int exposureLength,ackTime=0;

		exposureLength = darkCommand.getExposureTime();
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength);
		ackTime = (exposureLength+1000);
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}


	/**
	 * This method implements the DARK command. 
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
		DARK darkCommand = (DARK)command;
		DARK_DONE darkDone = new DARK_DONE(command.getId());
		MultdarkCommandThread multdarkThreadList[] =
			new MultdarkCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int exposureLength,cLayerCount,threadFinishedCount;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(darkCommand,darkDone) == true)
			return darkDone;
	// move the fold mirror to a dark location
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFoldToDark(darkCommand,darkDone) == false)
			return darkDone;
		if(testAbort(darkCommand,darkDone) == true)
			return darkDone;
		// get dark data
		exposureLength = darkCommand.getExposureTime();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+".");
		// get fits headers
		clearFitsHeaders();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(darkCommand,darkDone) == false)
			return darkDone;
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(darkCommand,darkDone) == false)
			return darkDone;
		if(testAbort(darkCommand,darkDone) == true)
			return darkDone;
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
				// exposureCount = 1
				multdarkThreadList[cLayerIndex].setMultdarkCommandParameters(exposureLength,1);
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
			darkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+600);
			darkDone.setErrorString(this.getClass().getName()+
						    ":processCommand:sendMultdarkCommand failed:"+e);
			darkDone.setSuccessful(false);
			return darkDone;
		}
		// setup return values.
		// setup multdark done
		// standard success values
		darkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		darkDone.setErrorString("");
		darkDone.setSuccessful(true);
		// check to see whether any of the C layer threw an exception.
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multdarkThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":processCommand:C Layer "+cLayerIndex+
					     " returned exception:",multdarkThreadList[cLayerIndex].getException());
				darkDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+601);
				darkDone.setErrorString(this.getClass().getName()+
							":processCommand:MULTDARK C Layer "+cLayerIndex+
							" returned exception:"+
							multdarkThreadList[cLayerIndex].getException());
				darkDone.setSuccessful(false);
			}
			else
			{
				// this c Layer multdark was successful
				// If this is C layer 0 (which is on the same machine as the Java layer),
				// pull last filename. This means the filename returned to the IcsGUI can be
				// displayed by it
				if(cLayerIndex == 0)
					darkDone.setFilename(multdarkThreadList[cLayerIndex].getLastFilename());
			}
		}// end for on cLayerIndex
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return darkDone;
	}
}
