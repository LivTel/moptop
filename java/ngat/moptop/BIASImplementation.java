// BIASImplementation.java
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
 * This class provides the implementation for the BIAS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.moptop.HardwareImplementation
 */
public class BIASImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public BIASImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.BIAS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.BIAS";
	}

	/**
	 * This method returns the MULTBIAS command's acknowledge time. 
	 * <ul>
	 * <li>The acknowledge time is a nominal readout time (1s).
	 * <li>The default acknowledge time is added to the total and returned.
	 * </ul>
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		BIAS biasCommand = (BIAS)command;
		ACK acknowledge = null;
		int ackTime=0;

		ackTime = 1000; // 1 second
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the BIAS command. 
	 * <ul>
	 * <li>It moves the fold mirror to a dark location.
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). A filtered subset
	 *     is sent on to the C layer.
	 * <li>A thread of class MultbiasCommandThread, is instantiated, and started, for each C Layer.
	 *     Each thread sends the multbias command to it's C layer, and then waits for an answer, 
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
	 * @see MultbiasCommandThread
	 * @see MultbiasCommandThread#setMoptop
	 * @see MultbiasCommandThread#setStatus
	 * @see MultbiasCommandThread#setCLayerIndex
	 * @see MultbiasCommandThread#setMultbiasCommandParameters
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		BIAS biasCommand = (BIAS)command;
		BIAS_DONE biasDone = new BIAS_DONE(command.getId());
		MultbiasCommandThread multbiasThreadList[] =
			new MultbiasCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int cLayerCount,threadFinishedCount;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(biasCommand,biasDone) == true)
			return biasDone;
	// move the fold mirror to a bias location
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFoldToDark(biasCommand,biasDone) == false)
			return biasDone;
		if(testAbort(biasCommand,biasDone) == true)
			return biasDone;
		// get fits headers
		clearFitsHeaders();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(biasCommand,biasDone) == false)
			return biasDone;
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(biasCommand,biasDone) == false)
			return biasDone;
		if(testAbort(biasCommand,biasDone) == true)
			return biasDone;
		// call multdark command
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting MULTBIAS C layer command threads.");
		try
		{
			// start multbias command threads - one per c layer
			cLayerCount = status.getPropertyInteger("moptop.c.count");
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{
				multbiasThreadList[cLayerIndex] = new MultbiasCommandThread();
				multbiasThreadList[cLayerIndex].setMoptop(moptop);
				multbiasThreadList[cLayerIndex].setStatus(status);
				multbiasThreadList[cLayerIndex].setCLayerIndex(cLayerIndex);
				multbiasThreadList[cLayerIndex].setMultbiasCommandParameters(1);// exposureCount = 1
				moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":processCommand:Starting MULTBIAS C layer command thread "+cLayerIndex+".");
				multbiasThreadList[cLayerIndex].start();
			}
			// wait for all multbias command threads to terminate
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:Waiting for MULTBIAS C layer command threads to terminate.");
			do
			{
				threadFinishedCount = 0;
				for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
				{
					if(multbiasThreadList[cLayerIndex].isAlive())
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTBIAS C layer command thread "+
							   cLayerIndex+" still running.");
					}
					else
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTBIAS C layer command thread "+
							   cLayerIndex+" has finished.");
						threadFinishedCount++;
					}
					// sleep a bit
					Thread.sleep(1000);
				}// end for
			} while(threadFinishedCount < cLayerCount);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:All MULTBIAS C layer command threads have finished.");
		}
		catch(Exception e )
		{
			moptop.error(this.getClass().getName()+":processCommand:Multbias command thread failed:",e);
			biasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+700);
			biasDone.setErrorString(this.getClass().getName()+
						":processCommand:sendMultbiasCommand failed:"+e);
			biasDone.setSuccessful(false);
			return biasDone;
		}
		// setup return values.
		// setup multbias done
		// standard success values
		biasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		biasDone.setErrorString("");
		biasDone.setSuccessful(true);
		// check to see whether any of the C layer threw an exception.
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multbiasThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":processCommand:C Layer "+cLayerIndex+
					     " returned exception:",multbiasThreadList[cLayerIndex].getException());
				biasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+701);
				biasDone.setErrorString(this.getClass().getName()+
							    ":processCommand:MULTBIAS C Layer "+cLayerIndex+
							    " returned exception:"+
							    multbiasThreadList[cLayerIndex].getException());
				biasDone.setSuccessful(false);
			}
			else
			{
				// this c Layer multbias was successful
				// If this is C layer 0 (which is on the same machine as the Java layer),
				// pull last filename. This means the filename returned to the IcsGUI can be
				// displayed by it
				if(cLayerIndex == 0)
					biasDone.setFilename(multbiasThreadList[cLayerIndex].getLastFilename());
			}
		}// end for on cLayerIndex
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return biasDone;
	}
}
