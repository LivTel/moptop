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

	/**
	 * Inner class that extends thread. Used to send multdark commands to an individual C layer.
	 */
	protected class MultdarkCommandThread extends Thread
	{
		/**
		 * Which C layer we are interacting with.
		 */
		int cLayerIndex;
		/**
		 * The Multdark exposure length, in milliseconds.
		 */
		int exposureLength;
		/**
		 * How many exposures to do in a Multdark.
		 */
		int exposureCount;
		/**
		 * Return value from C layer: how many FITS files were produced.
		 */
		int filenameCount;
		/**
		 * Return value from C layer: The multdark number of the FITS files.
		 */
		int multdarkNumber;
		/**
		 * Return value from C layer: The last FITS filename produced.
		 */
		String lastFilename;
		/**
		 * The exception thrown by the Multdark Command, if an error occurs.
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
		 * Set the Multdark command parameters.
		 * @param expLen The exposure length in milliseconds.
		 * @param expCount The number of exposures.
		 * @see #exposureLength
		 * @see #exposureCount
		 */
		public void setMultdarkCommandParameters(int expLen,int expCount)
		{
			exposureLength = expLen;
			exposureCount = expCount;
		}

		/**
		 * The run method. This should be called after setCLayerIndex and setMultdarkCommandParameters have
		 * been called. sendMultdarkCommand is called, and if it throws an exception this is captured and
		 * stored in exception.
		 * @see #sendMultdarkCommand
		 * @see #exception
		 */
		public void run()
		{
			try
			{
				sendMultdarkCommand();
			}
			catch(Exception e)
			{
				exception = e;
			}
		}

		/**
		 * Get the last FITS image filename generated by the completed multdark command.
		 * @return A string containing the last FITS image filename generated by the completed multdark command.
		 * @see #lastFilename
		 */
		public String getLastFilename()
		{
			return lastFilename;
		}

		/**
		 * Get an exception thrown by the multdark command.
		 * @return The exception thrown as a result of the multdark command, or null if no exception occurred.
		 * @see #exception
		 */
		public Exception getException()
		{
			return exception;
		}

		/**
		 * Send the multdark command to the C layer.
		 * @exception Exception Thrown if an error occurs.
		 * @see #cLayerIndex
		 * @see #exposureLength
		 * @see #exposureCount
		 * @see #parseSuccessfulReply
		 * @see ngat.moptop.command.MultdarkCommand
		 * @see ngat.moptop.command.MultdarkCommand#setAddress
		 * @see ngat.moptop.command.MultdarkCommand#setPortNumber
		 * @see ngat.moptop.command.MultdarkCommand#setCommand
		 * @see ngat.moptop.command.MultdarkCommand#sendCommand
		 * @see ngat.moptop.command.MultdarkCommand#getParsedReplyOK
		 * @see ngat.moptop.command.MultdarkCommand#getReturnCode
		 * @see ngat.moptop.command.MultdarkCommand#getParsedReply
		 */
		protected void sendMultdarkCommand() throws Exception
		{
			MultdarkCommand command = null;
			int portNumber,returnCode;
			String hostname = null;
			String errorString = null;
			
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:"+
				   "\n\t:cLayerIndex = "+cLayerIndex+
				   "\n\t:exposureLength = "+exposureLength+
				   "\n\t:exposureCount = "+exposureCount+".");
			command = new MultdarkCommand();
			// configure C comms
			hostname = status.getProperty("moptop.c.hostname."+cLayerIndex);
			portNumber = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			command.setAddress(hostname);
			command.setPortNumber(portNumber);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:cLayerIndex = "+cLayerIndex+
				   " :hostname = "+hostname+" :port number = "+portNumber+".");
			command.setCommand(exposureLength,exposureCount);
			// actually send the command to the C layer
			command.sendCommand();
			// check the parsed reply
			if(command.getParsedReplyOK() == false)
			{
				returnCode = command.getReturnCode();
				errorString = command.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "sendMultrunCommand:multdark command failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":sendMultdarkCommand:Command failed with return code "+returnCode+
						    " and error string:"+errorString);
			}
			// extract data from successful reply.
			parseSuccessfulReply(command.getParsedReply());
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:finished.");
		}

		/**
		 * Parse the successful reply string from the Multdark command.
		 * Currently should be of the form:
		 * "&lt;filename count&gt; &lt;multdark number&gt; &lt;last FITS filename&gt;".
		 * The preceeding '0' denoting success should have already been stripped off.
		 * @param replyString The reply string.
		 * @exception NumberFormatException Thrown if parsing the filename count or multdark number fails.
		 * @see #filenameCount
		 * @see #multdarkNumber
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
					multdarkNumber = Integer.parseInt(token);
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
	}// MultdarkCommandThread inner class
}
