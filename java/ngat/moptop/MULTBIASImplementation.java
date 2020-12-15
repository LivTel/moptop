// MULTBIASImplementation.java
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
 * This class provides the implementation for the MULTBIAS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.moptop.HardwareImplementation
 */
public class MULTBIASImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public MULTBIASImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTBIAS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTBIAS";
	}

	/**
	 * This method returns the MULTBIAS command's acknowledge time. 
	 * <ul>
	 * <li>The acknowledge time is a nominal readout time (1s) multiplied by the exposure count.
	 * <li>The default acknowledge time is added to the total and returned.
	 * </ul>
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTBIAS#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTBIAS multBiasCommand = (MULTBIAS)command;
		ACK acknowledge = null;
		int exposureCount,ackTime=0;

		exposureCount = multBiasCommand.getNumberExposures();
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureCount = "+exposureCount);
		ackTime = 1000*exposureCount;
		moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTBIAS command. 
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
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTBIAS multBiasCommand = (MULTBIAS)command;
		MULTBIAS_DONE multBiasDone = new MULTBIAS_DONE(command.getId());
		MultbiasCommandThread multbiasThreadList[] =
			new MultbiasCommandThread[MoptopConstants.MOPTOP_MAX_C_LAYER_COUNT];
		int exposureCount,cLayerCount,threadFinishedCount;

		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multBiasCommand,multBiasDone) == true)
			return multBiasDone;
	// move the fold mirror to a bias location
		moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFoldToDark(multBiasCommand,multBiasDone) == false)
			return multBiasDone;
		if(testAbort(multBiasCommand,multBiasDone) == true)
			return multBiasDone;
		// get multbias data
		exposureCount = multBiasCommand.getNumberExposures();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureCount = "+exposureCount+".");
		// get fits headers
		clearFitsHeaders();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multBiasCommand,multBiasDone) == false)
			return multBiasDone;
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multBiasCommand,multBiasDone) == false)
			return multBiasDone;
		if(testAbort(multBiasCommand,multBiasDone) == true)
			return multBiasDone;
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
				multbiasThreadList[cLayerIndex].setCLayerIndex(cLayerIndex);
				multbiasThreadList[cLayerIndex].setMultbiasCommandParameters(exposureCount);
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
			multBiasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2600);
			multBiasDone.setErrorString(this.getClass().getName()+
						    ":processCommand:sendMultbiasCommand failed:"+e);
			multBiasDone.setSuccessful(false);
			return multBiasDone;
		}
		// setup return values.
		// setup multbias done
		// standard success values
		multBiasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		multBiasDone.setErrorString("");
		multBiasDone.setSuccessful(true);
		// check to see whether any of the C layer threw an exception.
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// if we threw an exception, log exception and setup failed multRunDone
			if(multbiasThreadList[cLayerIndex].getException() != null)
			{
				moptop.error(this.getClass().getName()+":processCommand:C Layer "+cLayerIndex+
					     " returned exception:",multbiasThreadList[cLayerIndex].getException());
				multBiasDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2601);
				multBiasDone.setErrorString(this.getClass().getName()+
							    ":processCommand:MULTBIAS C Layer "+cLayerIndex+
							    " returned exception:"+
							    multbiasThreadList[cLayerIndex].getException());
				multBiasDone.setSuccessful(false);
			}
			else
			{
				// this c Layer multdark was successful
				// If this is C layer 0 (which is on the same machine as the Java layer),
				// pull last filename. This means the filename returned to the IcsGUI can be
				// displayed by it
				if(cLayerIndex == 0)
					multBiasDone.setFilename(multbiasThreadList[cLayerIndex].getLastFilename());
			}
		}// end for on cLayerIndex
	// return done object.
		moptop.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multBiasDone;
	}

	/**
	 * Inner class that extends thread. Used to send multbias commands to an individual C layer.
	 */
	protected class MultbiasCommandThread extends Thread
	{
		/**
		 * Which C layer we are interacting with.
		 */
		int cLayerIndex;
		/**
		 * How many exposures to do in a Multbias.
		 */
		int exposureCount;
		/**
		 * Return value from C layer: how many FITS files were produced.
		 */
		int filenameCount;
		/**
		 * Return value from C layer: The multbias number of the FITS files.
		 */
		int multbiasNumber;
		/**
		 * Return value from C layer: The last FITS filename produced.
		 */
		String lastFilename;
		/**
		 * The exception thrown by the Multbias Command, if an error occurs.
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
		 * Set the Multbias command parameters.
		 * @param expCount The number of exposures.
		 * @see #exposureCount
		 */
		public void setMultbiasCommandParameters(int expCount)
		{
			exposureCount = expCount;
		}

		/**
		 * The run method. This should be called after setCLayerIndex and setMultbiasCommandParameters have
		 * been called. sendMultbiasCommand is called, and if it throws an exception this is captured and
		 * stored in exception.
		 * @see #sendMultbiasCommand
		 * @see #exception
		 */
		public void run()
		{
			try
			{
				sendMultbiasCommand();
			}
			catch(Exception e)
			{
				exception = e;
			}
		}

		/**
		 * Get the last FITS image filename generated by the completed multbias command.
		 * @return A string containing the last FITS image filename generated by the completed multbias command.
		 * @see #lastFilename
		 */
		public String getLastFilename()
		{
			return lastFilename;
		}

		/**
		 * Get an exception thrown by the multbias command.
		 * @return The exception thrown as a result of the multbias command, or null if no exception occurred.
		 * @see #exception
		 */
		public Exception getException()
		{
			return exception;
		}

		/**
		 * Send the multbias command to the C layer.
		 * @exception Exception Thrown if an error occurs.
		 * @see #cLayerIndex
		 * @see #exposureCount
		 * @see #parseSuccessfulReply
		 * @see ngat.moptop.command.MultbiasCommand
		 * @see ngat.moptop.command.MultbiasCommand#setAddress
		 * @see ngat.moptop.command.MultbiasCommand#setPortNumber
		 * @see ngat.moptop.command.MultbiasCommand#setCommand
		 * @see ngat.moptop.command.MultbiasCommand#sendCommand
		 * @see ngat.moptop.command.MultbiasCommand#getParsedReplyOK
		 * @see ngat.moptop.command.MultbiasCommand#getReturnCode
		 * @see ngat.moptop.command.MultbiasCommand#getParsedReply
		 */
		protected void sendMultbiasCommand() throws Exception
		{
			MultbiasCommand command = null;
			int portNumber,returnCode;
			String hostname = null;
			String errorString = null;
			
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:"+
				   "\n\t:cLayerIndex = "+cLayerIndex+
				   "\n\t:exposureCount = "+exposureCount+".");
			command = new MultbiasCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			command.setCommand(exposureCount);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendMultrunCommand:multbias command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendMultbiasCommand:Command failed with return code "+returnCode+
						    " and error string:"+errorString);
			}
			// extract data from successful reply.
			parseSuccessfulReply(command.getParsedReply());
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:finished.");
		}

		/**
		 * Parse the successful reply string from the Multbias command.
		 * Currently should be of the form:
		 * "&lt;filename count&gt; &lt;multdark number&gt; &lt;last FITS filename&gt;".
		 * The preceeding '0' denoting success should have already been stripped off.
		 * @param replyString The reply string.
		 * @exception NumberFormatException Thrown if parsing the filename count or multbias number fails.
		 * @see #filenameCount
		 * @see #multbiasNumber
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
					multbiasNumber = Integer.parseInt(token);
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
	}// MultbiasCommandThread inner class
}
