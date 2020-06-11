// GET_STATUSImplementation.java
// $Id$
package ngat.moptop;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.moptop.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;
import ngat.util.ExecuteCommand;

/**
 * This class provides the implementation for the GET_STATUS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 */
public class GET_STATUSImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * This hashtable is created in processCommand, and filled with status data,
	 * and is returned in the GET_STATUS_DONE object.
	 * Could be declared:  Generic:&lt;String, Object&gt; but this is not supported by Java 1.4.
	 */
	private Hashtable hashTable = null;
	/**
	 * Standard status string passed back in the hashTable, describing the detector temperature status health,
	 * using the standard keyword KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS. 
	 * Initialised to VALUE_STATUS_UNKNOWN.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_UNKNOWN
	 */
	private String detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
	/**
	 * The number of C layers we have to communicate with.
	 */
	protected int cLayerCount;
	/**
	 * A list of hostname strings, one per C layer, to send C layer commands to.
	 */
	protected Vector cLayerHostnameList = null;
	/**
	 * A list of port numbers, one per C layer, to send C layer commands to.
	 */
	protected Vector cLayerPortNumberList = null;
	/**
	 * The current overall mode (status) of the RingoIII control system.
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_IDLE
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_EXPOSING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_READING_OUT
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_ERROR
	 */
	protected int currentMode;

	/**
	 * Constructor.
	 */
	public GET_STATUSImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.GET_STATUS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.GET_STATUS";
	}

	/**
	 * This method gets the GET_STATUS command's acknowledge time. 
	 * This takes the default acknowledge time to implement.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see MoptopTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the GET_STATUS command. 
	 * The local hashTable is setup (returned in the done object) and a local copy of status setup.
	 * <ul>
	 * <li>getCLayerConfig is called to get the C layer address/port number.
	 * <li>getExposureStatus is called to get the exposure status into the exposureStatus and exposureStatusString
	 *     variables.
	 * <li>"Exposure Status" and "Exposure Status String" status properties are added to the hashtable.
	 * <li>The "Instrument" status property is set to the "moptop.get_status.instrument_name" property value.
	 * <li>The detectorTemperatureInstrumentStatus is initialised.
	 * <li>The "currentCommand" status hashtable value is set to the currently executing command.
	 * <li>getStatusExposureIndex / getStatusExposureCount / getStatusExposureMultrun / getStatusExposureRun / 
	 *     getStatusExposureWindow / getStatusExposureLength / 
	 *     getStatusExposureStartTime are called to add some basic status to the hashtable.
	 * <li>getIntermediateStatus is called if the GET_STATUS command level is at least intermediate.
	 * <li>getFullStatusis called if the GET_STATUS command level is at least full.
	 * </ul>
	 * An object of class GET_STATUS_DONE is returned, with the information retrieved.
	 * @param command The GET_STATUS command.
	 * @return An object of class GET_STATUS_DONE is returned.
	 * @see #moptop
	 * @see #status
	 * @see #hashTable
	 * @see #detectorTemperatureInstrumentStatus
	 * @see #getCLayerConfig
	 * @see #getExposureStatus
	 * @see #getStatusExposureIndex
	 * @see #getStatusExposureCount
	 * @see #getStatusExposureMultrun
	 * @see #getStatusExposureRun
	 * @see #getStatusExposureWindow
	 * @see #getStatusExposureLength
	 * @see #getStatusExposureStartTime
	 * @see #getIntermediateStatus
	 * @see #getFullStatus
	 * @see #currentMode
	 * @see MoptopStatus#getProperty
	 * @see MoptopStatus#getCurrentCommand
	 * @see GET_STATUS#getLevel
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		GET_STATUS getStatusCommand = (GET_STATUS)command;
		GET_STATUS_DONE getStatusDone = new GET_STATUS_DONE(command.getId());
		ISS_TO_INST currentCommand = null;

		try
		{
			// Create new hashtable to be returned
			// v1.5 generic typing of collections:<String, Object>, can't be used due to v1.4 compatibility
			hashTable = new Hashtable();
			// get C layer comms configuration
			getCLayerConfig();
			// exposure status (per C layer)
			// Also sets currentMode as an aggregate of C layers
			getExposureStatus(); 
			getStatusDone.setCurrentMode(currentMode); 
			// What instrument is this?
			hashTable.put("Instrument",status.getProperty("moptop.get_status.instrument_name"));
			// Initialise Standard status to UNKNOWN
			detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_UNKNOWN;
			hashTable.put(GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
				      detectorTemperatureInstrumentStatus);
			hashTable.put(GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,GET_STATUS_DONE.VALUE_STATUS_UNKNOWN);
			// current command
			currentCommand = status.getCurrentCommand();
			if(currentCommand == null)
				hashTable.put("currentCommand","");
			else
				hashTable.put("currentCommand",currentCommand.getClass().getName());
			// basic information
			getFilterWheelStatus();
			// per c layer
			// "Exposure Count" is searched for by the IcsGUI
			getStatusExposureCount(); 
			// "Exposure Length" is needed for IcsGUI
			getStatusExposureLength();  
			// "Exposure Start Time" is needed for IcsGUI
			getStatusExposureStartTime(); 
			// "Elapsed Exposure Time" is needed for IcsGUI.
			// This requires "Exposure Length" and "Exposure Start Time hashtable entries to have
			// already been inserted by getStatusExposureLength/getStatusExposureStartTime
			getStatusExposureElapsedTime();
			// "Exposure Number" is added in getStatusExposureIndex
			getStatusExposureIndex(); 
			getStatusExposureMultrun(); 
			getStatusExposureRun();
			getStatusExposureWindow();
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+
				       ":processCommand:Retrieving basic status failed.",e);
			getStatusDone.setDisplayInfo(hashTable);
			getStatusDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+2500);
			getStatusDone.setErrorString("processCommand:Retrieving basic status failed:"+e);
			getStatusDone.setSuccessful(false);
			return getStatusDone;
		}
	// intermediate level information - basic plus controller calls.
		if(getStatusCommand.getLevel() >= GET_STATUS.LEVEL_INTERMEDIATE)
		{
			getIntermediateStatus();
		}// end if intermediate level status
	// Get full status information.
		if(getStatusCommand.getLevel() >= GET_STATUS.LEVEL_FULL)
		{
			getFullStatus();
		}
	// set hashtable and return values.
		getStatusDone.setDisplayInfo(hashTable);
		getStatusDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_NO_ERROR);
		getStatusDone.setErrorString("");
		getStatusDone.setSuccessful(true);
	// return done object.
		return getStatusDone;
	}

	/**
	 * Get the C layer's hostname and port numberfrom the properties into some internal variables. 
	 * These are also added into the returned hashtable data.
	 * @exception Exception Thrown if the relevant property can be received.
	 * @see #status
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see MoptopStatus#getProperty
	 * @see MoptopStatus#getPropertyInteger
	 */
	protected void getCLayerConfig() throws Exception
	{
		String s = null;
		int i;

		// C Layer
		cLayerCount = status.getPropertyInteger("moptop.c.count");
		hashTable.put("C.Layer.Count",new Integer(cLayerCount));
		cLayerHostnameList = new Vector(cLayerCount);
		cLayerPortNumberList = new Vector(cLayerCount);
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			// get C layer data
			s = status.getProperty("moptop.c.hostname."+cLayerIndex);
			i = status.getPropertyInteger("moptop.c.port_number."+cLayerIndex);
			// Add to GET_STATus vectors
			cLayerHostnameList.add(cLayerIndex,s);
			cLayerPortNumberList.add(cLayerIndex,new Integer(i));
			// Add to returned Hashtable
			hashTable.put("C.Layer.Hostname."+cLayerIndex,new String(s));
			hashTable.put("C.Layer.Port.Number."+cLayerIndex,new Integer(i));
		}
	}

	/**
	 * Get the status/position of the filter wheel.
	 * @exception Exception Thrown if an error occurs.
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusFilterWheelFilterCommand
	 * @see ngat.moptop.command.StatusFilterWheelPositionCommand
	 * @see ngat.moptop.command.StatusFilterWheelStatusCommand
	 */
	protected void getFilterWheelStatus() throws Exception
	{
		StatusFilterWheelFilterCommand statusFilterWheelFilterCommand = null;
		StatusFilterWheelPositionCommand statusFilterWheelPositionCommand = null;
		StatusFilterWheelStatusCommand statusFilterWheelStatusCommand = null;
		String cLayerHostname = null;
		String errorString = null;
		String filterName = null;
		String filterWheelStatus = null;
		int returnCode,cLayerIndex,cLayerPortNumber,filterWheelPosition;

		// The filter wheel is attached to C layer 0 only
		cLayerIndex = 0;
		// "status filterwheel filter" command
		cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
		cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getFilterWheelStatus:started for C layer Index:"+
			   cLayerIndex+":Hostname: "+cLayerHostname+" Port Number: "+cLayerPortNumber+".");
		// Setup StatusFilterWheelFilterCommand
		statusFilterWheelFilterCommand = new StatusFilterWheelFilterCommand();
		statusFilterWheelFilterCommand.setAddress(cLayerHostname);
		statusFilterWheelFilterCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusFilterWheelFilterCommand.sendCommand();
		// check the parsed reply
		if(statusFilterWheelFilterCommand.getParsedReplyOK() == false)
		{
			returnCode = statusFilterWheelFilterCommand.getReturnCode();
			errorString = statusFilterWheelFilterCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getFilterWheelStatus:filter wheel filter command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getFilterWheelStatus:filter wheel filter command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
		}
		filterName = statusFilterWheelFilterCommand.getFilterName();
		hashTable.put("Filter Wheel:1",new String(filterName));
		// "status filterwheel position" command
		statusFilterWheelPositionCommand = new StatusFilterWheelPositionCommand();
		statusFilterWheelPositionCommand.setAddress(cLayerHostname);
		statusFilterWheelPositionCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusFilterWheelPositionCommand.sendCommand();
		// check the parsed reply
		if(statusFilterWheelPositionCommand.getParsedReplyOK() == false)
		{
			returnCode = statusFilterWheelPositionCommand.getReturnCode();
			errorString = statusFilterWheelPositionCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getFilterWheelStatus:filter wheel position command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getFilterWheelStatus:filter wheel position command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		filterWheelPosition = statusFilterWheelPositionCommand.getFilterWheelPosition();
		hashTable.put("Filter Wheel Position:1",new Integer(filterWheelPosition));
		// "status filterwheel status" command
		statusFilterWheelStatusCommand = new StatusFilterWheelStatusCommand();
		statusFilterWheelStatusCommand.setAddress(cLayerHostname);
		statusFilterWheelStatusCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusFilterWheelStatusCommand.sendCommand();
		// check the parsed reply
		if(statusFilterWheelStatusCommand.getParsedReplyOK() == false)
		{
			returnCode = statusFilterWheelStatusCommand.getReturnCode();
			errorString = statusFilterWheelStatusCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getFilterWheelStatus:filter wheel status command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getFilterWheelStatus:filter wheel status command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		filterWheelStatus = statusFilterWheelStatusCommand.getFilterWheelStatus();
		hashTable.put("Filter Wheel Status:1",new String(filterWheelStatus));
	}
	
	/**
	 * Get the exposure status. 
	 * This retrieved using an instance of StatusExposureStatusCommand for each C layer.
	 * The "Multrun In Progress."+cLayerIndex keyword/value pairs are generated from the returned status. 
	 * The currentMode is set as either MODE_IDLE, or MODE_EXPOSING if a C layer reports a multrun is in progress.
	 * @exception Exception Thrown if an error occurs.
	 * @see #currentMode
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureStatusCommand
	 * @see ngat.moptop.command.StatusExposureStatusCommand#getMultrunInProgress
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_IDLE
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_EXPOSING
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#MODE_ERROR
	 */
	protected void getExposureStatus() throws Exception
	{
		StatusExposureStatusCommand statusCommand = null;
		String cLayerHostname = null;
		int returnCode,cLayerPortNumber;
		String errorString = null;
		boolean multrunInProgress;
		
		// initialise currentMode to IDLE
		currentMode = GET_STATUS_DONE.MODE_IDLE;
		// loop over C layer indexes
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			// Setup StatusExposureStatusCommand for this C layer
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getExposureStatus:started for C layer Index:"+
				   cLayerIndex+":Hostname: "+cLayerHostname+" Port Number: "+cLayerPortNumber+".");
			statusCommand = new StatusExposureStatusCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getExposureStatus:exposure status command for C layer Index:"+
					   cLayerIndex+" failed with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":getExposureStatus:exposure status command for C layer Index:"+
						    cLayerIndex+" failed with return code "+
						    returnCode+" and error string:"+errorString);
			}
			multrunInProgress = statusCommand.getMultrunInProgress();
			hashTable.put("Multrun In Progress."+cLayerIndex,new Boolean(multrunInProgress));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getExposureStatus:finished for C layer Index:"+
				   cLayerIndex+" with multrun in progress:"+multrunInProgress);
			// change currentMode dependant on what this C layer is doing
			// We can only detect whether a multrun is in progress in Moptop
			// If _any_ C layer thinks a multrun is in progress, change status to MODE_EXPOSING
			if(currentMode == GET_STATUS_DONE.MODE_IDLE)
			{
				if(multrunInProgress)
					currentMode = GET_STATUS_DONE.MODE_EXPOSING;
			}
		}// end for on C layer
	}

	/**
	 * Get the exposure count. An instance of StatusExposureCountCommand is used to send the command
	 * to each C layer, The returned value is stored in
	 * the hashTable, under the "Exposure Count."+cLayerIndex key. If all the return counts are the same,
	 * the hashTable entry "Exposure Count" is updated with the aggregate returned value.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureCountCommand
	 */
	protected void getStatusExposureCount() throws Exception
	{
		StatusExposureCountCommand statusCommand = null;
		int returnCode,exposureCount,cLayerPortNumber;
		int lastExposureCount = -1;
		String cLayerHostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureCount:started.");
		// loop over C layer indexes
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureCount:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureCountCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getStatusExposureCount:exposure count command failed for C layer Index "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":getStatusExposureCount:"+
						    "exposure count command failed for C layer Index "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			// Keep track of whether the exposureCount returned from each C layer is the same.
			// lastExposureCount is set to the first returned exposureCount, if subsequent
			// returned exposureCounts do not match the first lastExposureCount is set to -1.
			exposureCount = statusCommand.getExposureCount();
			if(cLayerIndex == 0)
				lastExposureCount = exposureCount;
			else if(exposureCount != lastExposureCount)
				lastExposureCount = -1;
			hashTable.put("Exposure Count."+cLayerIndex,new Integer(exposureCount));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureCount:finished for C layer Index "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with count:"+exposureCount);
		}// end for on cLayerIndex
		// overall Exposure Count needed for IcsGUI
		// only set if they were all the same.
		if(lastExposureCount != -1)
			hashTable.put("Exposure Count",new Integer(lastExposureCount));
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureCount:"+
			   "finished with overall Exposure Count "+lastExposureCount+".");
	}

	/**
	 * Get the exposure length. An instance of StatusExposureLengthCommand is used to send the command
	 * to each C layer. The returned value is stored in
	 * the hashTable, under the "Exposure Length,"+cLayerIndex key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureLengthCommand
	 */
	protected void getStatusExposureLength() throws Exception
	{
		StatusExposureLengthCommand statusCommand = null;
		int returnCode,exposureLength,cLayerPortNumber;
		int lastExposureLength = -1;
		String cLayerHostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureLength:started.");
		// loop over C layer indexes
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureLength:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureLengthCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getStatusExposureLength:exposure length command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getStatusExposureLength:"+
						    "exposure length command failed for C layer "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			exposureLength = statusCommand.getExposureLength();
			// keep track of whether all exposureLength's for each C layer are the same
			// Set lastExposureLength to the first exposureLength
			// if subsequent exposure lengths do not match lastExposureLength set lastExposureLength to -1
			if(cLayerIndex == 0)
				lastExposureLength = exposureLength;
			else if(lastExposureLength != exposureLength)
				lastExposureLength = -1;
			hashTable.put("Exposure Length."+cLayerIndex,new Integer(exposureLength));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureLength:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with length:"+
				   exposureLength+ " ms.");
		}// end for on cLayerIndex
		// if all the exposure lengths for each C layer are the same, put the result in
		// the "Exposure Length" key, which is read by the IcsGUI
		if(lastExposureLength != -1)
			hashTable.put("Exposure Length",new Integer(lastExposureLength));
	}

	/**
	 * Get the exposure start time. An instance of StatusExposureStartTimeCommand is used to send the command
	 * to each C layer. The returned value is stored in
	 * the hashTable, under the "Exposure Start Time" and "Exposure Start Time Date" key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureStartTimeCommand
	 */
	protected void getStatusExposureStartTime() throws Exception
	{
		StatusExposureStartTimeCommand statusCommand = null;
		Date exposureStartTime = null;
		String cLayerHostname = null;
		int returnCode,cLayerPortNumber;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureStartTime:started.");
		// loop over C layer indexes
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureStartTime:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureStartTimeCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,"getStatusExposureStartTime:"+
					   "exposure start time command failed for C layer "+cLayerIndex+
					   " ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getStatusExposureStartTime:"+
						    "exposure start time command failed for C layer "+cLayerIndex+
						    " ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
						    returnCode+" and error string:"+errorString);
			}
			exposureStartTime = statusCommand.getTimestamp();
			hashTable.put("Exposure Start Time."+cLayerIndex,new Long(exposureStartTime.getTime()));
			hashTable.put("Exposure Start Time Date."+cLayerIndex,exposureStartTime);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureStartTime:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with start time:"+
				   exposureStartTime);
		} // end for on cLayerIndex
		// all exposure times should be roughly the same, so we add the last one for the IcsGUI to read
		hashTable.put("Exposure Start Time",new Long(exposureStartTime.getTime()));
		hashTable.put("Exposure Start Time Date",exposureStartTime);
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureStartTime:finished with start time:"+
			   exposureStartTime);
	}

	/**
	 * Compute and insert the "Elapsed Exposure Time" key. 
	 * The IcsgUI calculates the remaining exposure time by doing the calculation(exposureLength-elapsedExposureTime).
	 * Here, we do the following:
	 * <ul>
	 * <li>We get the current time nowTime.
	 * <li>We initialise the largestElapsedExposureTime to -1.
	 * <li>We loop over the C layers:
	 *     <ul>
	 *     <li>We retrieve the "Exposure Start Time."+cLayerIndex hashtable value 
	 *         (previously generated in getStatusExposureStartTime).
	 *     <li>We retrieve the "Exposure Length."+cLayerIndex hashtable value 
	 *         (previously generated in getStatusExposureLength).
	 *     <li>We compute the elapsed exposure time as being nowTime minus the exposure start time.
	 *     <li>If this elapsed exposure time is the largest so far, we store it.
	 *     </ul>
	 * <li>We set the "Elapsed Exposure Time" hashTable key to the computed largest elapsed exposure time.
	 * </ul>
	 * @see #hashTable
	 * @see #getStatusExposureStartTime
	 * @see #getStatusExposureLength
	 */
	protected void getStatusExposureElapsedTime()
	{
		Date nowTime = null;
		Integer exposureLengthObject = null;
		Long exposureStartTimeObject = null;
		long exposureStartTime[] = new long[cLayerCount];
	        int exposureLength[] = new int[cLayerCount];
	        int elapsedExposureTime[] = new int[cLayerCount];
		int largestElapsedExposureTime;
		
		nowTime = new Date();
		largestElapsedExposureTime = -1;
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			exposureStartTimeObject = (Long)(hashTable.get("Exposure Start Time."+cLayerIndex));
			exposureLengthObject = (Integer)(hashTable.get("Exposure Length."+cLayerIndex));
			exposureStartTime[cLayerIndex] = exposureStartTimeObject.longValue();
			exposureLength[cLayerIndex] = exposureLengthObject.intValue();
			elapsedExposureTime[cLayerIndex] = (int)(nowTime.getTime()-exposureStartTime[cLayerIndex]);
			hashTable.put("Elapsed Exposure Time."+cLayerIndex,new Integer(elapsedExposureTime[cLayerIndex]));
			if(elapsedExposureTime[cLayerIndex] > largestElapsedExposureTime)
				largestElapsedExposureTime = elapsedExposureTime[cLayerIndex];
		}// end for on cLayerIndex
		hashTable.put("Elapsed Exposure Time",new Integer(largestElapsedExposureTime));
	}
	
	/**
	 * Get the exposure index for each camera. 
	 * An instance of StatusExposureIndexCommand is used to send the command to each C layer. 
	 * The returned values are stored in the hashTable, under the :
	 * "Exposure Index."+cLayerIndex key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see #hashTable
	 * @see ngat.moptop.command.StatusExposureIndexCommand
	 */
	protected void getStatusExposureIndex() throws Exception
	{
		StatusExposureIndexCommand statusCommand = null;
		int returnCode,exposureIndex;
		String cLayerHostname = null;
		int cLayerPortNumber = -1;
		int lastExposureIndex = -1;
		String errorString = null;

		// loop over C layer indexes
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureIndex:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureIndexCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getStatusExposureIndex:exposure index command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+
						    ":getStatusExposureIndex:exposure index command failed for C layer "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			exposureIndex = statusCommand.getExposureIndex();
			hashTable.put("Exposure Index."+cLayerIndex,new Integer(exposureIndex));
			// exposure number is really the same thing, but is used by the IcsGUI.
			hashTable.put("Exposure Number."+cLayerIndex,new Integer(exposureIndex));
			// are all exposureIndex's the same?
			if(cLayerIndex == 0)
			{
				lastExposureIndex = exposureIndex;
			}
			else
			{
				if(exposureIndex != lastExposureIndex)
					lastExposureIndex = -1;
			}
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureIndex:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with index:"+exposureIndex);
		}// end for on cLayerIndex
		// if all exposure Indexes for all c layers are the same, add an overall exposureNumber for the IcsGUI
		// to pick up
		if(lastExposureIndex > -1)
		{
			hashTable.put("Exposure Number",new Integer(lastExposureIndex));
			moptop.log(Logging.VERBOSITY_VERBOSE,"getStatusExposureIndex:Added overall Exposure Number :"+
				   lastExposureIndex);
		}
	}

	/**
	 * Get the exposure multrun. An instance of StatusExposureMultrunCommand is used to send the command
	 * to each C layer. The returned value is stored inthe hashTable, under the "Exposure Multrun."+cLayerIndex key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureMultrunCommand
	 */
	protected void getStatusExposureMultrun() throws Exception
	{
		StatusExposureMultrunCommand statusCommand = null;
		int returnCode,exposureMultrun,cLayerPortNumber;
		String cLayerHostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureMultrun:started.");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureMultrun:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureMultrunCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					  "getStatusExposureMultrun:exposure multrun command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getStatusExposureMultrun:"+
						    "exposure multrun command failed for C layer "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			exposureMultrun = statusCommand.getExposureMultrun();
			hashTable.put("Exposure Multrun."+cLayerIndex,
				      new Integer(exposureMultrun));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureMultrun:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with multrun number:"+
				   exposureMultrun);
		}// end for on cameraIndex
	}

	/**
	 * Get the exposure multrun run. An instance of StatusExposureRunCommand is used to send the command
	 * to each C layer. The returned value is stored in the hashTable, under the "Exposure Run."+cLayerIndex key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureRunCommand
	 */
	protected void getStatusExposureRun() throws Exception
	{
		StatusExposureRunCommand statusCommand = null;
		int returnCode,exposureRun,cLayerPortNumber;
		String cLayerHostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureRun:started.");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureRun:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureRunCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getStatusExposureRun:exposure run command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getStatusExposureRun:"+
						    "exposure run command failed for C layer "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			exposureRun = statusCommand.getExposureRun();
			hashTable.put("Exposure Run."+cLayerIndex,new Integer(exposureRun));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureRun:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
				   ") with run number:"+exposureRun);
		}// end for on cameraIndex
	}

	/**
	 * Get the exposure multrun window. An instance of StatusExposureWindowCommand is used to send the command
	 * to to each C layer. The returned values are stored in the hashTable, under the 
	 * "Exposure Window."+cLayerIndex key.
	 * @exception Exception Thrown if an error occurs.
	 * @see #hashTable
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusExposureWindowCommand
	 */
	protected void getStatusExposureWindow() throws Exception
	{
		StatusExposureWindowCommand statusCommand = null;
		int returnCode,exposureWindow,cLayerPortNumber;
		String cLayerHostname = null;
		String errorString = null;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureWindow:started.");
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureWindow:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusExposureWindowCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getStatusExposureWindow:exposure window command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+") with return code "+
					   returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getStatusExposureWindow:"+
						    "exposure window command failed for C layer "+cLayerIndex+
						    " ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			exposureWindow = statusCommand.getExposureWindow();
			hashTable.put("Exposure Window."+cLayerIndex,new Integer(exposureWindow));
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getStatusExposureWindow:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
				   ") with window number:"+exposureWindow);
		}// end for on cameraIndex
	}
	
	/**
	 * Get intermediate level status. This is:
	 * <ul>
	 * <li>CCD temperature information from each camera.
	 * <li>Rotator status.
	 * </ul>
	 * The overall health and well-being statii are then computed using setInstrumentStatus.
	 * @see #getTemperature
	 * @see #getRotatorStatus
	 * @see #setInstrumentStatus
	 */
	private void getIntermediateStatus()
	{
		try
		{
			// per clayer
			getTemperature();
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+
				     ":getIntermediateStatus:Retrieving temperature status failed.",e);
		}
		try
		{
			// The rotator is on C layer 0 only
			getRotatorStatus();
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+
				     ":getIntermediateStatus:Retrieving rotator status failed.",e);
		}
	// Standard status
		setInstrumentStatus();
	}

	/**
	 * Get the current, or C layer cached, CCD temperature.
	 * An instance of StatusTemperatureGetCommand is used to send the command
	 * to each C layer. The returned value is stored in
	 * the hashTable, under the "Temperature."+cLayerIndex key (converted to Kelvin). 
	 * A timestamp is also retrieved (when the temperature was actually measured, it may be a cached value), 
	 * and this is stored in the "Temperature Timestamp."+cLayerIndex key.
	 * setDetectorTemperatureInstrumentStatus is called with a list of detector temperatures to set
	 * the CCD temperature health and wellbeing values.
	 * @exception Exception Thrown if an error occurs.
	 * @see #cLayerCount
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see #hashTable
	 * @see #setDetectorTemperatureInstrumentStatus
	 * @see ngat.moptop.Moptop#CENTIGRADE_TO_KELVIN
	 * @see ngat.moptop.command.StatusTemperatureGetCommand
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#setAddress
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#setPortNumber
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#sendCommand
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#getReturnCode
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#getParsedReply
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#getTemperature
	 * @see ngat.moptop.command.StatusTemperatureGetCommand#getTimestamp
	 */
	protected void getTemperature() throws Exception
	{
		StatusTemperatureGetCommand statusCommand = null;
		String cLayerHostname = null;
		int returnCode,cLayerPortNumber;
		String errorString = null;
		double temperatureList[];
		Date timestamp;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getTemperature:started.");
		temperatureList = new double[cLayerCount];
		for(int cLayerIndex = 0; cLayerIndex < cLayerCount; cLayerIndex++)
		{
			cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
			cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getTemperature:started for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+").");
			statusCommand = new StatusTemperatureGetCommand();
			statusCommand.setAddress(cLayerHostname);
			statusCommand.setPortNumber(cLayerPortNumber);
			// actually send the command to the C layer
			statusCommand.sendCommand();
			// check the parsed reply
			if(statusCommand.getParsedReplyOK() == false)
			{
				returnCode = statusCommand.getReturnCode();
				errorString = statusCommand.getParsedReply();
				moptop.log(Logging.VERBOSITY_TERSE,
					   "getTemperature:exposure run command failed for C layer "+
					   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
					   ") with return code "+returnCode+" and error string:"+errorString);
				throw new Exception(this.getClass().getName()+":getTemperature:"+
						    "exposure run command failed C layer "+
						    cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
						    ") with return code "+returnCode+" and error string:"+errorString);
			}
			temperatureList[cLayerIndex] = statusCommand.getTemperature();
			timestamp = statusCommand.getTimestamp();
			hashTable.put("Temperature."+cLayerIndex,
				      new Double(temperatureList[cLayerIndex]+Moptop.CENTIGRADE_TO_KELVIN));
			hashTable.put("Temperature Timestamp."+cLayerIndex,timestamp);
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getTemperature:finished for C layer "+
				   cLayerIndex+" ("+cLayerHostname+":"+cLayerPortNumber+
				   ") with temperature:"+temperatureList[cLayerIndex]+
				   " measured at "+timestamp);
		}// end for on cameraIndex
		// set aggregate temperature status
		setDetectorTemperatureInstrumentStatus(temperatureList);
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getTemperature:finished.");
	}

	/**
	 * Set the standard entry for detector temperature in the hashtable based upon the current temperature.
	 * Reads the folowing config:
	 * <ul>
	 * <li>moptop.get_status.detector.temperature.warm.warn
	 * <li>moptop.get_status.detector.temperature.warm.fail
	 * <li>moptop.get_status.detector.temperature.cold.warn
	 * <li>moptop.get_status.detector.temperature.cold.fail
	 * </ul>
	 * @param temperatureList An array of current CCD temperature in degrees C, one per camera head.
	 * @exception NumberFormatException Thrown if the config is not a valid double.
	 * @see #hashTable
	 * @see #status
	 * @see #detectorTemperatureInstrumentStatus
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_OK
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_WARN
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_FAIL
	 */
	protected void setDetectorTemperatureInstrumentStatus(double temperatureList[]) 
		throws NumberFormatException
	{
		String perDetectorTemperatureStatus = null;
		double warmWarnTemperature,warmFailTemperature,coldWarnTemperature,coldFailTemperature;

		// get config for warn and fail temperatures
		warmWarnTemperature = status.getPropertyDouble("moptop.get_status.detector.temperature.warm.warn");
		warmFailTemperature = status.getPropertyDouble("moptop.get_status.detector.temperature.warm.fail");
		coldWarnTemperature = status.getPropertyDouble("moptop.get_status.detector.temperature.cold.warn");
		coldFailTemperature = status.getPropertyDouble("moptop.get_status.detector.temperature.cold.fail");
		// initialise status to OK
		detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		// set status, based on each camera head, if the camera head status is worse
		// than the current status value i.e. set the overall status to the worse of the camera's statii.
		for(int i = 0; i < temperatureList.length; i++)
		{
			// initialse per-detector status to OK
			perDetectorTemperatureStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
			if(temperatureList[i] > warmFailTemperature)
			{
				detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
				perDetectorTemperatureStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			}
			else if(temperatureList[i] > warmWarnTemperature)
			{
				// only set to WARN if we are currently OKAY (i.e. if we are FAIL stay FAIL) 
				if(detectorTemperatureInstrumentStatus == GET_STATUS_DONE.VALUE_STATUS_OK)
				{
					detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
				}
				if(perDetectorTemperatureStatus == GET_STATUS_DONE.VALUE_STATUS_OK)
				{
					perDetectorTemperatureStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
				}
			}
			else if(temperatureList[i] < coldFailTemperature)
			{
				detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
				perDetectorTemperatureStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
			}
			else if(temperatureList[i] < coldWarnTemperature)
			{
				if(detectorTemperatureInstrumentStatus == GET_STATUS_DONE.VALUE_STATUS_OK)
				{
					// only set to WARN if we are currently OKAY (i.e. if we are FAIL stay FAIL) 
					detectorTemperatureInstrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
				}
				if(perDetectorTemperatureStatus == GET_STATUS_DONE.VALUE_STATUS_OK)
				{
					perDetectorTemperatureStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
				}
			}
			// set per-detector temperature status
			hashTable.put(GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS+"."+i,
				      perDetectorTemperatureStatus);
		}// end for
		// set hashtable entry
		hashTable.put(GET_STATUS_DONE.KEYWORD_DETECTOR_TEMPERATURE_INSTRUMENT_STATUS,
			      detectorTemperatureInstrumentStatus);
	}

	/**
	 * Set the overall instrument status keyword in the hashtable. This is derived from sub-system keyword values,
	 * currently only the detector temperature. HashTable entry KEYWORD_INSTRUMENT_STATUS)
	 * should be set to the worst of OK/WARN/FAIL. If sub-systems are UNKNOWN, OK is returned.
	 * @see #hashTable
	 * @see #status
	 * @see #detectorTemperatureInstrumentStatus
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#KEYWORD_INSTRUMENT_STATUS
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_OK
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_WARN
	 * @see ngat.message.ISS_INST.GET_STATUS_DONE#VALUE_STATUS_FAIL
	 */
	protected void setInstrumentStatus()
	{
		String instrumentStatus;

		// default to OK
		instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_OK;
		// if a sub-status is in warning, overall status is in warning
		if(detectorTemperatureInstrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_WARN))
			instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_WARN;
		// if a sub-status is in fail, overall status is in fail. This overrides a previous warn
	        if(detectorTemperatureInstrumentStatus.equals(GET_STATUS_DONE.VALUE_STATUS_FAIL))
			instrumentStatus = GET_STATUS_DONE.VALUE_STATUS_FAIL;
		// set standard status in hashtable
		hashTable.put(GET_STATUS_DONE.KEYWORD_INSTRUMENT_STATUS,instrumentStatus);
	}

	/**
	 * Get the speed/status/position of the rotator.
	 * @exception Exception Thrown if an error occurs.
	 * @see #cLayerHostnameList
	 * @see #cLayerPortNumberList
	 * @see ngat.moptop.command.StatusRotatorSpeedCommand
	 * @see ngat.moptop.command.StatusRotatorPositionCommand
	 * @see ngat.moptop.command.StatusRotatorStatusCommand
	 */
	protected void getRotatorStatus() throws Exception
	{
		StatusRotatorSpeedCommand statusRotatorSpeedCommand = null;
		StatusRotatorPositionCommand statusRotatorPositionCommand = null;
		StatusRotatorStatusCommand statusRotatorStatusCommand = null;
		String cLayerHostname = null;
		String errorString = null;
		String rotatorSpeed = null;
		String rotatorStatus = null;
		int returnCode,cLayerIndex,cLayerPortNumber;
		double rotatorPosition;
		
		// The rotator is attached to C layer 0 only
		cLayerIndex = 0;
		// "status rotator speed" command
		cLayerHostname = (String)(cLayerHostnameList.get(cLayerIndex));
		cLayerPortNumber = ((Integer)(cLayerPortNumberList.get(cLayerIndex))).intValue();
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,"getRotatorStatus:started for C layer Index:"+
			   cLayerIndex+":Hostname: "+cLayerHostname+" Port Number: "+cLayerPortNumber+".");
		// Setup StatusRotatorSpeedCommand
		statusRotatorSpeedCommand = new StatusRotatorSpeedCommand();
		statusRotatorSpeedCommand.setAddress(cLayerHostname);
		statusRotatorSpeedCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusRotatorSpeedCommand.sendCommand();
		// check the parsed reply
		if(statusRotatorSpeedCommand.getParsedReplyOK() == false)
		{
			returnCode = statusRotatorSpeedCommand.getReturnCode();
			errorString = statusRotatorSpeedCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getRotatorStatus:rotator speed command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getRotatorStatus:rotator speed command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
		}
		rotatorSpeed = statusRotatorSpeedCommand.getRotatorSpeed();
		hashTable.put("Rotator Speed",new String(rotatorSpeed));
		// "status rotator position" command
		statusRotatorPositionCommand = new StatusRotatorPositionCommand();
		statusRotatorPositionCommand.setAddress(cLayerHostname);
		statusRotatorPositionCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusRotatorPositionCommand.sendCommand();
		// check the parsed reply
		if(statusRotatorPositionCommand.getParsedReplyOK() == false)
		{
			returnCode = statusRotatorPositionCommand.getReturnCode();
			errorString = statusRotatorPositionCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getRotatorStatus:rotator position command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getRotatorStatus:rotator position command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		rotatorPosition = statusRotatorPositionCommand.getRotatorPosition();
		hashTable.put("Rotator Position",new Double(rotatorPosition));
		// "status rotator status" command
		statusRotatorStatusCommand = new StatusRotatorStatusCommand();
		statusRotatorStatusCommand.setAddress(cLayerHostname);
		statusRotatorStatusCommand.setPortNumber(cLayerPortNumber);
		// actually send the command to the C layer
		statusRotatorStatusCommand.sendCommand();
		// check the parsed reply
		if(statusRotatorStatusCommand.getParsedReplyOK() == false)
		{
			returnCode = statusRotatorStatusCommand.getReturnCode();
			errorString = statusRotatorStatusCommand.getParsedReply();
			moptop.log(Logging.VERBOSITY_TERSE,
				   "getRotatorStatus:rotator status command for C layer Index:"+
				   cLayerIndex+" failed with return code "+returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":getRotatorStatus:rotator status command for C layer Index:"+
					    cLayerIndex+" failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		rotatorStatus = statusRotatorStatusCommand.getRotatorStatus();
		hashTable.put("Rotator Status",new String(rotatorStatus));
	}
	
	/**
	 * Method to get misc status, when level FULL has been selected.
	 * The following data is put into the hashTable:
	 * <ul>
	 * <li><b>Log Level</b> The current logging level Moptop is using.
	 * <li><b>Disk Usage</b> The results of running a &quot;df -k&quot;, to get the disk usage.
	 * <li><b>Process List</b> The results of running a &quot;ps -e -o pid,pcpu,vsz,ruser,stime,time,args&quot;, 
	 * 	to get the processes running on this machine.
	 * <li><b>Uptime</b> The results of running a &quot;uptime&quot;, 
	 * 	to get system load and time since last reboot.
	 * <li><b>Total Memory, Free Memory</b> The total and free memory in the Java virtual machine.
	 * <li><b>java.version, java.vendor, java.home, java.vm.version, java.vm.vendor, java.class.path</b> 
	 * 	Java virtual machine version, classpath and type.
	 * <li><b>os.name, os.arch, os.version</b> The operating system type/version.
	 * <li><b>user.name, user.home, user.dir</b> Data about the user the process is running as.
	 * <li><b>thread.list</b> A list of threads the Moptop process is running.
	 * </ul>
	 * @see #serverConnectionThread
	 * @see #hashTable
	 * @see ExecuteCommand#run
	 * @see MoptopStatus#getLogLevel
	 */
	private void getFullStatus()
	{
		ExecuteCommand executeCommand = null;
		Runtime runtime = null;
		StringBuffer sb = null;
		Thread threadList[] = null;
		int threadCount;

		// log level
		hashTable.put("Log Level",new Integer(status.getLogLevel()));
		// execute 'df -k' on instrument computer
		executeCommand = new ExecuteCommand("df -k");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Disk Usage",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Disk Usage",new String(executeCommand.getException().toString()));
		// execute "ps -e -o pid,pcpu,vsz,ruser,stime,time,args" on instrument computer
		executeCommand = new ExecuteCommand("ps -e -o pid,pcpu,vsz,ruser,stime,time,args");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Process List",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Process List",new String(executeCommand.getException().toString()));
		// execute "uptime" on instrument computer
		executeCommand = new ExecuteCommand("uptime");
		executeCommand.run();
		if(executeCommand.getException() == null)
			hashTable.put("Uptime",new String(executeCommand.getOutputString()));
		else
			hashTable.put("Uptime",new String(executeCommand.getException().toString()));
		// get vm memory situation
		runtime = Runtime.getRuntime();
		hashTable.put("Free Memory",new Long(runtime.freeMemory()));
		hashTable.put("Total Memory",new Long(runtime.totalMemory()));
		// get some java vm information
		hashTable.put("java.version",new String(System.getProperty("java.version")));
		hashTable.put("java.vendor",new String(System.getProperty("java.vendor")));
		hashTable.put("java.home",new String(System.getProperty("java.home")));
		hashTable.put("java.vm.version",new String(System.getProperty("java.vm.version")));
		hashTable.put("java.vm.vendor",new String(System.getProperty("java.vm.vendor")));
		hashTable.put("java.class.path",new String(System.getProperty("java.class.path")));
		hashTable.put("os.name",new String(System.getProperty("os.name")));
		hashTable.put("os.arch",new String(System.getProperty("os.arch")));
		hashTable.put("os.version",new String(System.getProperty("os.version")));
		hashTable.put("user.name",new String(System.getProperty("user.name")));
		hashTable.put("user.home",new String(System.getProperty("user.home")));
		hashTable.put("user.dir",new String(System.getProperty("user.dir")));
	}
}
