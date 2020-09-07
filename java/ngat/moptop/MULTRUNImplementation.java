// MULTRUNImplementation.java
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
 * This class provides the implementation for the MULTRUN command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.moptop.HardwareImplementation
 */
public class MULTRUNImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public MULTRUNImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTRUN&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTRUN";
	}

	/**
	 * This method returns the MULTRUN command's acknowledge time. 
	 * <ul>
	 * <li>If the MULTRUN exposure length is greater than zero, the acknowledge time is the exposure length,
	 * <li>If the MULTRUN exposure count is greater than zero, 
	 *     each exposure count tells the C layer to do one rotation's worth of exposures.
	 *     The acknowledge time is the exposure count multiplied by 80 
	 *     (at 4.5 deg/s the slowest length of time to do an exposure), 
	 *     multiplied by 1000 (acknowledge time is in milliseconds).
	 * <li>The default acknowledge time is added to the total and returned.
	 * </ul>
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTRUN#getExposureTime
	 * @see MULTRUN#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTRUN multRunCommand = (MULTRUN)command;
		ACK acknowledge = null;
		int exposureLength,exposureCount,ackTime=0;

		exposureLength = multRunCommand.getExposureTime();
		exposureCount = multRunCommand.getNumberExposures();
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount);
		if(exposureLength > 0)
			ackTime = exposureLength;
		else if(exposureCount > 0)
		{
			// exposureCount is rotationCount,
			// up to 8 seconds per rotation at 45 deg/s (fast)
			// up to 80 seconds per rotation at 4.5 deg/s (slow)
			ackTime = (exposureCount*80*1000);
		}
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTRUN command. 
	 * <ul>
	 * <li>It moves the fold mirror to the correct location.
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). A filtered subset
	 *     is sent on to the C layer.
	 * <li>We call doMultrunSetupCommands to call multrun_setup on each C layer, check the result,
	 *     and fix any multrun mismatches.
	 * <li>A thread of class MultrunCommandThread, is instantiated, and started, for each C Layer.
	 *     Each thread sends the multrun command to it's C layer, and then waits for an answer, 
	 *     and parses the result.
	 * <li>We enter a loop monitoring the threads, until they have all terminated.
	 * <li>The done object is setup. We check whether any of the C layer threads threw an exception, 
	 *     during execution.
	 * </ul>
	 * @see #testAbort
	 * @see ngat.moptop.HardwareImplementation#moveFold
	 * @see ngat.moptop.HardwareImplementation#clearFitsHeaders
	 * @see ngat.moptop.HardwareImplementation#setFitsHeaders
	 * @see ngat.moptop.HardwareImplementation#getFitsHeadersFromISS
	 * @see ngat.moptop.MoptopConstants#MOPTOP_MAX_C_LAYER_COUNT
	 * @see MultrunCommandThread
	 * @see #doMultrunSetupCommands
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTRUN multRunCommand = (MULTRUN)command;
		MULTRUN_ACK multRunAck = null;
		MULTRUN_DONE multRunDone = new MULTRUN_DONE(command.getId());
		MultrunCommandThread multrunThreadList[] =
			new MultrunCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int exposureLength,exposureCount,cLayerCount,threadFinishedCount;
		boolean standard;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
	// move the fold mirror to the correct location
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFold(multRunCommand,multRunDone) == false)
			return multRunDone;
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
		// get multrun data
		exposureLength = multRunCommand.getExposureTime();
		exposureCount = multRunCommand.getNumberExposures();
		standard = multRunCommand.getStandard();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount+" :standard = "+standard+".");
		// get fits headers
		clearFitsHeaders();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multRunCommand,multRunDone) == false)
			return multRunDone;
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multRunCommand,multRunDone) == false)
			return multRunDone;
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
		// call multrun setup command
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multrun Setup C layer command threads.");
		if(doMultrunSetupCommands(multRunCommand,multRunDone) == false)
			return multRunDone;
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
		// call multrun command
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting MULTRUN C layer command threads.");
		try
		{
			// start multrun command threads - one per c layer
			cLayerCount = status.getPropertyInteger("moptop.c.count");
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{
				multrunThreadList[cLayerIndex] = new MultrunCommandThread();
				multrunThreadList[cLayerIndex].setCLayerIndex(cLayerIndex);
				multrunThreadList[cLayerIndex].setMultrunCommandParameters(exposureLength,
										    exposureCount,standard);
				moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":processCommand:Starting MULTRUN C layer command thread "+cLayerIndex+".");
				multrunThreadList[cLayerIndex].start();
			}
			//wait for all multrun command threads to terminate
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:Waiting for MULTRUN C layer command threads to terminate.");
			do
			{
				threadFinishedCount = 0;
				for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
				{
					if(multrunThreadList[cLayerIndex].isAlive())
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTRUN C layer command thread "+
							   cLayerIndex+" still running.");
					}
					else
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":processCommand:MULTRUN C layer command thread "+
							   cLayerIndex+" has finished.");
						threadFinishedCount++;
					}
					// sleep a bit
					Thread.sleep(1000);
				}// end for
			} while(threadFinishedCount < cLayerCount);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":processCommand:All MULTRUN C layer command threads have finished.");
		}
		catch(Exception e )
		{
			moptop.error(this.getClass().getName()+":processCommand:Multrun command thread failed:",e);
			multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+900);
			multRunDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultrunCommand failed:"+e);
			multRunDone.setSuccessful(false);
			return multRunDone;
		}
		// setup return values.
		// setup multrun done
		// set to some blank value
		multRunDone.setSeeing(0.0f);
		multRunDone.setCounts(0.0f);
		multRunDone.setXpix(0.0f);
		multRunDone.setYpix(0.0f);
		multRunDone.setPhotometricity(0.0f);
		multRunDone.setSkyBrightness(0.0f);
		multRunDone.setSaturation(false);
		// standard success values
		multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		multRunDone.setErrorString("");
		multRunDone.setSuccessful(true);
		// check to see whether any of the C layer threw an exception.
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multrunThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":processCommand:C Layer "+cLayerIndex+
					     " returned exception:",multrunThreadList[cLayerIndex].getException());
				multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+901);
				multRunDone.setErrorString(this.getClass().getName()+
							   ":processCommand:MULTRUN C Layer "+cLayerIndex+
							   " returned exception:"+
							   multrunThreadList[cLayerIndex].getException());
				multRunDone.setSuccessful(false);
			}
			else
			{
				// this c Layer multrun was successful
				// If this is C layer 0 (which is on the same machine as the Java layer),
				// pull last filename. This means the filename returned to the IcsGUI can be
				// displayed by it
				if(cLayerIndex == 0)
					multRunDone.setFilename(multrunThreadList[cLayerIndex].getLastFilename());
			}
		}// end for on cLayerIndex
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multRunDone;
	}

	/**
	 * Call the multrun_setup command on each C layer.
	 * <ul>
	 * <li>A thread of class MultrunSetupCommandThread, is instantiated, and started, for each C Layer.
	 *     Each thread sends the multrun setup command to it's C layer, and then waits for an answer, 
	 *     and parses the result.
	 * <li>We enter a loop monitoring the threads, until they have all terminated.
	 * <li>We check whether any of the C layer threads threw an exception, during execution.
	 * <li>We look at all the multrun numbers returned by the multrun_setup commands, and find the maximum.
	 * <li>We loop over the C layers, and check the multrun number returned are all the maximum. If they are not,
	 *     we call fixMultrunNumber on that C layer.
	 * </ul>
	 * @param multRunCommand The instance of the MULTRUN command being implemented.
	 * @param multRunDone The instance of MULTRUN_DONE. If an error occurs, the error message will appear in this
	 *        object, to be returned to the client.
	 * @return The method returns true on success, and false if an error occurs.
	 * @see MultrunSetupCommandThread
	 * @see #fixMultrunNumber
	 */
	protected boolean doMultrunSetupCommands(MULTRUN multRunCommand,MULTRUN_DONE multRunDone)
	{
		MultrunSetupCommandThread multrunSetupThreadList[] =
			new MultrunSetupCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int cLayerCount,threadFinishedCount,maxMultrunNumber;
		
		try
		{
			// start multrun setup command threads - one per c layer
			cLayerCount = status.getPropertyInteger("moptop.c.count");
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{
				multrunSetupThreadList[cLayerIndex] = new MultrunSetupCommandThread();
				multrunSetupThreadList[cLayerIndex].setCLayerIndex(cLayerIndex);
				moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":doMultrunSetupCommands:Starting Multrun Setup C layer command thread "+
					   cLayerIndex+".");
				multrunSetupThreadList[cLayerIndex].start();
			}
			//wait for all multrun command threads to terminate
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":doMultrunSetupCommands:Waiting for Multrun Setup C layer command threads to terminate.");
			do
			{
				threadFinishedCount = 0;
				for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
				{
					if(multrunSetupThreadList[cLayerIndex].isAlive())
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":doMultrunSetupCommands:Multrun Setup C layer command thread "+
							   cLayerIndex+" still running.");
					}
					else
					{
						moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
							   ":doMultrunSetupCommands:Multrun Setup C layer command thread "+
							   cLayerIndex+" has finished.");
						threadFinishedCount++;
					}
					// sleep a bit
					Thread.sleep(1000);
				}// end for
			} while(threadFinishedCount < cLayerCount);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":doMultrunSetupCommands:All Multrun Setup C layer command threads have finished.");
		}
		catch(Exception e )
		{
			moptop.error(this.getClass().getName()+":doMultrunSetupCommands:Multrun Setup command thread failed:",
				     e);
			multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+902);
			multRunDone.setErrorString(this.getClass().getName()+
						   ":doMultrunSetupCommands:Multrun Setup Command failed:"+e);
			multRunDone.setSuccessful(false);
			return false;
		}
		// check to see whether any of the C layer threw an exception.
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":doMultrunSetupCommands:Check Multrun Setup C layer command thread replies.");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multrunSetupThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":doMultrunSetupCommands:Multrun Setup C Layer "+
					     cLayerIndex+" returned exception:",
					     multrunSetupThreadList[cLayerIndex].getException());
				multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+903);
				multRunDone.setErrorString(this.getClass().getName()+
							   ":doMultrunSetupCommands:Multrun Setup C Layer "+cLayerIndex+
							   " returned exception:"+
							   multrunSetupThreadList[cLayerIndex].getException());
				multRunDone.setSuccessful(false);
				return false;
			}
		}// end for on cLayerIndex
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":doMultrunSetupCommands:Check Multrun Setup multrun numbers - find the maximum.");
		maxMultrunNumber = -1;
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// We don't need this test as we have already failed the multrun if this is false
			//if(multrunSetupThreadList[cLayerIndex].getException() == null)
			//{
			if(multrunSetupThreadList[cLayerIndex].getMultrunNumber() > maxMultrunNumber)
				maxMultrunNumber = multrunSetupThreadList[cLayerIndex].getMultrunNumber();
			//}
		}
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":doMultrunSetupCommands:Check Multrun Setup multrun numbers are all the same ("+
			   maxMultrunNumber+").");
		try
		{
			for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
			{		
				if(multrunSetupThreadList[cLayerIndex].getMultrunNumber() != maxMultrunNumber)
				{
					fixMultrunNumber(cLayerIndex,multrunSetupThreadList[cLayerIndex].getMultrunNumber(),
							 maxMultrunNumber);
				}
			}// end for on cLayerIndex
		}
		catch(Exception e )
		{
			moptop.error(this.getClass().getName()+":doMultrunSetupCommands:fixMultrunNumber failed:",e);
			multRunDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+904);
			multRunDone.setErrorString(this.getClass().getName()+
						   ":doMultrunSetupCommands:fixMultrunNumber failed:"+e);
			multRunDone.setSuccessful(false);
			return false;
		}		
		return true;
	}

	/**
	 * This method should only be invoked on a C layer that has returned a multrun number (from multrun_setup)
	 * that is less than the maximum multrun number returned from any C layer. In this case, we invoke
	 * multrun_setup for this C layer until it's multrun number is the same as the maximum.
	 * @param cLayerIndex The C layer, whose multrun number is less than the maximum.
	 * @param currentMultrunNumber The multrun number returned by this C layer at the start of this method.
	 * @param targetMultrunNumber The maximum multrun number. 
	 * @exception Exception Thrown if an error occurs whilst catching up multrun numbers.
	 * @see MultrunSetupCommandThread
	 */
	protected void fixMultrunNumber(int cLayerIndex,int currentMultrunNumber,int targetMultrunNumber) throws Exception
	{
		MultrunSetupCommandThread multrunSetupThread = null;

		while(currentMultrunNumber < targetMultrunNumber)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":fixMultrunNumber:C layer command "+cLayerIndex+
				   " has multrun number "+currentMultrunNumber+" vs target "+targetMultrunNumber+".");
			multrunSetupThread = new MultrunSetupCommandThread();
			multrunSetupThread.setCLayerIndex(cLayerIndex);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":fixMultrunNumber:Starting Multrun Setup C layer command thread "+cLayerIndex+".");
			multrunSetupThread.start();
			while (multrunSetupThread.isAlive())
			{
				moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
					   ":fixMultrunNumber:Multrun Setup C layer command thread "+
					   cLayerIndex+" still running.");
				// sleep a bit
				Thread.sleep(1000);
			} // end while multrunSetupThread is running
			// If an exception occured, throw it with added information.
			if(multrunSetupThread.getException() != null)
			{
				throw new Exception(this.getClass().getName()+":fixMultrunNumber:Multrun Setup C Layer "+
						    cLayerIndex+" returned exception:",multrunSetupThread.getException());
			}
			if(multrunSetupThread.getMultrunNumber() <= currentMultrunNumber)
			{
				throw new Exception(this.getClass().getName()+":fixMultrunNumber:Multrun Setup C Layer "+
						    cLayerIndex+" did not increment it's multrun number ("+
						    multrunSetupThread.getMultrunNumber()+" vs "+currentMultrunNumber+").");
			}
			currentMultrunNumber = multrunSetupThread.getMultrunNumber();
		}// end while on currentMultrunNumber
	}
	
	/**
	 * Inner class that extends thread. Used to send multrun_setup commands to an individual C layer.
	 */
	protected class MultrunSetupCommandThread extends Thread
	{
		/**
		 * Which C layer we are interacting with.
		 */
		int cLayerIndex;
		/**
		 * Return value from C layer: The multrun number of the FITS files.
		 */
		int multrunNumber;
		/**
		 * The exception thrown by the Multrun Command, if an error occurs.
		 */
		Exception exception = null;

		/**
		 * Set the C layer index.
		 * @param i The index to use.
		 * @see #cLayerIndex
		 */
		public void setCLayerIndex(int i)
		{
			cLayerIndex = i;
		}

		/**
		 * The run method. This should be called after setCLayerIndex has
		 * been called. sendMultrunSetupCommand is called, and if it throws an exception this is captured and
		 * stored in exception.
		 * @see #sendMultrunSetupCommand
		 * @see #exception
		 */
		public void run()
		{
			try
			{
				sendMultrunSetupCommand();
			}
			catch(Exception e)
			{
				exception = e;
			}
		}

		/**
		 * Get the multrun number that the C layer is going to use for the _next_ multrun command.
		 * @return An integer, the multrun number that the C layer is going to use for the _next_ 
		 *         multrun command.
		 * @see #multrunNumber
		 */
		public int getMultrunNumber()
		{
			return multrunNumber;
		}

		/**
		 * Get an exception thrown by the multrun command.
		 * @return The exception thrown as a result of the multrun command, or null if no exception occurred.
		 * @see #exception
		 */
		public Exception getException()
		{
			return exception;
		}

		/**
		 * Send the multrun_setup command to the C layer.
		 * @exception Exception Thrown if an error occurs.
		 * @see #cLayerIndex
		 * @see ngat.moptop.command.MultrunSetupCommand
		 * @see ngat.moptop.command.MultrunSetupCommand#setAddress
		 * @see ngat.moptop.command.MultrunSetupCommand#setPortNumber
		 * @see ngat.moptop.command.MultrunSetupCommand#sendCommand
		 * @see ngat.moptop.command.MultrunSetupCommand#getParsedReplyOK
		 * @see ngat.moptop.command.MultrunSetupCommand#getReturnCode
		 * @see ngat.moptop.command.MultrunSetupCommand#getParsedReply
		 * @see ngat.moptop.command.MultrunSetupCommand#getMultrunNumber
		 */
		protected void sendMultrunSetupCommand() throws Exception
		{
			MultrunSetupCommand command = null;
			int portNumber,returnCode;
			String hostname = null;
			String errorString = null;
			
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunSetupCommand:"+
				   "\n\t:cLayerIndex = "+cLayerIndex+".");
			command = new MultrunSetupCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunSetupCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				multrunNumber = -1;
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					 "sendMultrunSetupSetupCommand:multrun_setup command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendMultrunSetupCommand:Command failed with return code "+
						    returnCode+" and error string:"+errorString);
			}
			// extract data from successful reply.
			multrunNumber = command.getMultrunNumber();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunSetupCommand:finished.");
		}
	}// MultrunSetupCommandThread inner class

	/**
	 * Inner class that extends thread. Used to send multrun commands to an individual C layer.
	 */
	protected class MultrunCommandThread extends Thread
	{
		/**
		 * Which C layer we are interacting with.
		 */
		int cLayerIndex;
		/**
		 * The Multrun exposure length, in milliseconds.
		 */
		int exposureLength;
		/**
		 * How many exposures to do in a Multrun.
		 */
		int exposureCount;
		/**
		 * Whether the Multrun is a standard, or not.
		 */
		boolean standard;
		/**
		 * Return value from C layer: how many FITS files were produced.
		 */
		int filenameCount;
		/**
		 * Return value from C layer: The multrun number of the FITS files.
		 */
		int multrunNumber;
		/**
		 * Return value from C layer: The last FITS filename produced.
		 */
		String lastFilename;
		/**
		 * The exception thrown by the Multrun Command, if an error occurs.
		 */
		Exception exception = null;

		/**
		 * Set the C layer index.
		 * @param i The index to use.
		 * @see #cLayerIndex
		 */
		public void setCLayerIndex(int i)
		{
			cLayerIndex = i;
		}

		/**
		 * Set the Multrun command parameters.
		 * @param expLen The exposure length in milliseconds.
		 * @param expCount The number of exposures.
		 * @param stan Whether the exposure is a standard, or not.
		 * @see #exposureLength
		 * @see #exposureCount
		 * @see #standard
		 */
		public void setMultrunCommandParameters(int expLen,int expCount,boolean stan)
		{
			exposureLength = expLen;
			exposureCount = expCount;
			standard = stan;
		}

		/**
		 * The run method. This should be called after setCLayerIndex and setMultrunCommandParameters have
		 * been called. sendMultrunCommand is called, and if it throws an exception this is captured and
		 * stored in exception.
		 * @see #sendMultrunCommand
		 * @see #exception
		 */
		public void run()
		{
			try
			{
				sendMultrunCommand();
			}
			catch(Exception e)
			{
				exception = e;
			}
		}

		/**
		 * Get the last FITS image filename generated by the completed multrun command.
		 * @return A string containing the last FITS image filename generated by the completed multrun command.
		 * @see #lastFilename
		 */
		public String getLastFilename()
		{
			return lastFilename;
		}

		/**
		 * Get an exception thrown by the multrun command.
		 * @return The exception thrown as a result of the multrun command, or null if no exception occurred.
		 * @see #exception
		 */
		public Exception getException()
		{
			return exception;
		}

		/**
		 * Send the multrun command to the C layer.
		 * @exception Exception Thrown if an error occurs.
		 * @see #cLayerIndex
		 * @see #exposureLength
		 * @see #exposureCount
		 * @see #standard
		 * @see #parseSuccessfulReply
		 * @see ngat.moptop.command.MultrunCommand
		 * @see ngat.moptop.command.MultrunCommand#setAddress
		 * @see ngat.moptop.command.MultrunCommand#setPortNumber
		 * @see ngat.moptop.command.MultrunCommand#setCommand
		 * @see ngat.moptop.command.MultrunCommand#sendCommand
		 * @see ngat.moptop.command.MultrunCommand#getParsedReplyOK
		 * @see ngat.moptop.command.MultrunCommand#getReturnCode
		 * @see ngat.moptop.command.MultrunCommand#getParsedReply
		 */
		protected void sendMultrunCommand() throws Exception
		{
			MultrunCommand command = null;
			int portNumber,returnCode;
			String hostname = null;
			String errorString = null;
			
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:"+
				   "\n\t:cLayerIndex = "+cLayerIndex+
				   "\n\t:exposureLength = "+exposureLength+
				   "\n\t:exposureCount = "+exposureCount+
				   "\n\t:standard = "+standard+".");
			command = new MultrunCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			command.setCommand(exposureLength,exposureCount,standard);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendMultrunCommand:multrun command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendMultrunCommand:Command failed with return code "+returnCode+
						    " and error string:"+errorString);
			}
			// extract data from successful reply.
			parseSuccessfulReply(command.getParsedReply());
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:finished.");
		}

		/**
		 * Parse the successful reply string from the Multrun command.
		 * Currently should be of the form:
		 * "&lt;filename count&gt; &lt;multrun number&gt; &lt;last FITS filename&gt;".
		 * The preceeding '0' denoting success should have already been stripped off.
		 * @param replyString The reply string.
		 * @exception NumberFormatException Thrown if parsing the filename count or multrun number fails.
		 * @see #filenameCount
		 * @see #multrunNumber
		 * @see #lastFilename
		 */
		protected void parseSuccessfulReply(String replyString) throws NumberFormatException
		{
			String token = null;
			StringTokenizer st = new StringTokenizer(replyString," ");
			int tokenIndex;
			
			moptop.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:started.");
			tokenIndex = 0;
			while(st.hasMoreTokens())
			{
				// get next token 
				token = st.nextToken();
				moptop.log(Logging.VERBOSITY_VERY_VERBOSE,"parseSuccessfulReply:token "+
					   tokenIndex+" = "+token+".");
				if(tokenIndex == 0)
				{
					filenameCount = Integer.parseInt(token);
				}
				else if(tokenIndex == 1)
				{
					multrunNumber = Integer.parseInt(token);
				}
				else if(tokenIndex == 2)
				{
					lastFilename = token;
				}
				else
				{
					moptop.log(Logging.VERBOSITY_VERBOSE,
						   "parseSuccessfulReply:unknown token index "+
						   tokenIndex+" = "+token+".");
				}
				// increment index
				tokenIndex++;
			}
			moptop.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:finished.");
		}
	}// MultrunCommandThread inner class
}
