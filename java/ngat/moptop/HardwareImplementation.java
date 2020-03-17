// HardwareImplementation.java
// $Header$
package ngat.moptop;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
//import ngat.moptop.command.*;
import ngat.util.logging.*;

/**
 * This class provides some common hardware related routines to move folds, and FITS
 * interface routines needed by many command implementations
 * @version $Revision: 1.1 $
 */
public class HardwareImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * This method calls the super-classes method. It then tries to fill in the reference to the hardware
	 * objects.
	 * @param command The command to be implemented.
	 */
	public void init(COMMAND command)
	{
		super.init(command);
	}
	/**
	 * This method is used to calculate how long an implementation of a command is going to take, so that the
	 * client has an idea of how long to wait before it can assume the server has died.
	 * @param command The command to be implemented.
	 * @return The time taken to implement this command, or the time taken before the next acknowledgement
	 * is to be sent.
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		return super.calculateAcknowledgeTime(command);
	}

	/**
	 * This routine performs the generic command implementation.
	 * @param command The command to be implemented.
	 * @return The results of the implementation of this command.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		return super.processCommand(command);
	}

	/**
	 * This routine tries to move the mirror fold to a certain location, by issuing a MOVE_FOLD command
	 * to the ISS. The position to move the fold to is specified by the moptop property file.
	 * If an error occurs the done objects field's are set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see MoptopStatus#getPropertyInteger
	 * @see Moptop#sendISSCommand
	 */
	public boolean moveFold(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		MOVE_FOLD moveFold = null;
		int mirrorFoldPosition = 0;

		moveFold = new MOVE_FOLD(command.getId());
		try
		{
			mirrorFoldPosition = status.getPropertyInteger("moptop.mirror_fold_position");
		}
		catch(NumberFormatException e)
		{
			mirrorFoldPosition = 0;
			moptop.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName(),e);
			done.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1201);
			done.setErrorString("moveFold:"+e);
			done.setSuccessful(false);
			return false;
		}
		moveFold.setMirror_position(mirrorFoldPosition);
		instToISSDone = moptop.sendISSCommand(moveFold,serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			moptop.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1202);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);		
			return false;
		}
		return true;
	}

	/**
	 * This routine clears the current set of FITS headers. 
	 */
	public void clearFitsHeaders()
	{
	}

	/**
	 * This routine gets a set of FITS header from a config file. The retrieved FITS headers are added to the 
	 * C layer. The "moptop.fits.keyword.&lt;n&gt;" properties is queried in ascending order
	 * of &lt;n&gt; to find keywords.
	 * The "moptop.fits.value.&lt;keyword&gt;.&lt;cMachineIndex&gt;.&lt;cameraIndex&gt;" property contains 
	 * the value of the keyword, which can be different for each camera head.
	 * The value's type is retrieved from the property "moptop.fits.value.type.&lt;keyword&gt;", 
	 * which should comtain one of the
	 * following values: boolean|float|integer|string.
	 * The addFitsHeader method is then called to actually add the FITS header to the C layer.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param commandDone A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #addFitsHeader
	 */
	public boolean setFitsHeaders(COMMAND command,COMMAND_DONE commandDone)
	{
		String keyword = null;
		String typeString = null;
		String valueString = null;
		boolean done;
		double dvalue;
		int index,ivalue,cameraCount,cLayerIndex,cameraIndex;
		boolean bvalue;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setFitsHeaders:Started.");
				cameraCount = status.getPropertyInteger("moptop.camera.count");
		index = 0;
		done = false;
		while(done == false)
		{
			moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":setFitsHeaders:Looking for keyword index "+index+" in list.");
			keyword = status.getProperty("moptop.fits.keyword."+index);
			if(keyword != null)
			{
				typeString = status.getProperty("moptop.fits.value.type."+keyword);
				if(typeString == null)
				{
					moptop.error(this.getClass().getName()+
						     ":setFitsHeaders:Failed to get value type for keyword:"+keyword);
					commandDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1203);
					commandDone.setErrorString(this.getClass().getName()+
						     ":setFitsHeaders:Failed to get value type for keyword:"+keyword);
					commandDone.setSuccessful(false);
					return false;
				}
				// Add FITS header to all cameras in all C layers
				for(int i = 0; i < cameraCount; i++)
				{
					cLayerIndex = status.getPropertyInteger("moptop.camera.c.layer."+i);
					cameraIndex = status.getPropertyInteger("moptop.camera.index."+i);
					try
					{
						if(typeString.equals("string"))
						{
							valueString = status.getProperty("moptop.fits.value."+keyword+
										      "."+cLayerIndex+"."+cameraIndex);
							addFitsHeader(cLayerIndex,cameraIndex,keyword,valueString);
						}
						else if(typeString.equals("integer"))
						{
							Integer iov = null;
							
							ivalue = status.getPropertyInteger("moptop.fits.value."+
											   keyword+"."+cLayerIndex+
											   "."+cameraIndex);
							iov = new Integer(ivalue);
							addFitsHeader(cLayerIndex,cameraIndex,keyword,iov);
						}
						else if(typeString.equals("float"))
						{
							Float fov = null;
							
							dvalue = status.getPropertyDouble("moptop.fits.value."+
											  keyword+"."+cLayerIndex+"."+
											  cameraIndex);
							fov = new Float(dvalue);
							addFitsHeader(cLayerIndex,cameraIndex,keyword,fov);
						}
						else if(typeString.equals("boolean"))
						{
							Boolean bov = null;
							
							bvalue = status.getPropertyBoolean("moptop.fits.value."+
											   keyword+"."+cLayerIndex+
											   "."+cameraIndex);
							bov = new Boolean(bvalue);
							addFitsHeader(cLayerIndex,cameraIndex,keyword,bov);
						}
						else
						{
							moptop.error(this.getClass().getName()+
								     ":setFitsHeaders:Unknown value type "+typeString+
								     " for keyword:"+keyword);
							commandDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+
										1204);
							commandDone.setErrorString(this.getClass().getName()+
										":setFitsHeaders:Unknown value type "+
										   typeString+" for keyword:"+keyword);
							commandDone.setSuccessful(false);
							return false;
						}
					}
					catch(Exception e)
					{
						moptop.error(this.getClass().getName()+
							     ":setFitsHeaders:Failed to add value for keyword:"+
							     keyword,e);
						commandDone.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1206);
						commandDone.setErrorString(this.getClass().getName()+
								 ":setFitsHeaders:Failed to add value for keyword:"+
									   keyword+":"+e);
						commandDone.setSuccessful(false);
						return false;
					}
				}// end for on cameraCount
				// increment index
				index++;
			}
			else
				done = true;
		}// end while
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setFitsHeaders:Finished.");
		return true;
	}

	/**
	 * This routine tries to get a set of FITS headers for an exposure, by issuing a GET_FITS command
	 * to the ISS. A subset of results from this command are put into the C layers list of FITS headers by calling
	 * addISSFitsHeaderList.
	 * If an error occurs the done objects field's can be set to record the error.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #addISSFitsHeaderList
	 * @see Moptop#sendISSCommand
	 * @see Moptop#getStatus
	 * @see MoptopStatus#getPropertyInteger
	 */
	public boolean getFitsHeadersFromISS(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		GET_FITS_DONE getFitsDone = null;
		FitsHeaderCardImage cardImage = null;
		Object value = null;
		Vector list = null;
		int orderNumberOffset;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":getFitsHeadersFromISS:Started.");
		instToISSDone = moptop.sendISSCommand(new GET_FITS(command.getId()),serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			moptop.error(this.getClass().getName()+":getFitsHeadersFromISS:"+
				     command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1205);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);
			return false;
		}
	// Get the returned FITS header information into the FitsHeader object.
		getFitsDone = (GET_FITS_DONE)instToISSDone;
	// extract specific FITS headers and add them to the C layers list
		list = getFitsDone.getFitsHeader();
		try
		{
			addISSFitsHeaderList(list);
		}
		catch(Exception e)
		{
			moptop.error(this.getClass().getName()+
				     ":getFitsHeadersFromISS:addISSFitsHeaderList failed.",e);
			done.setErrorNum(MoptopConstants.MOPTOP_ERROR_CODE_BASE+1207);
			done.setErrorString(this.getClass().getName()+
					    ":getFitsHeadersFromISS:addISSFitsHeaderList failed:"+e);
			done.setSuccessful(false);
			return false;
		}
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":getFitsHeadersFromISS:finished.");
		return true;
	}

	/**
	 * Try to extract a specified subset of the GET_FITS headers returned from the ISS (RCS),
	 * and pass them onto the C layer.
	 * The subset is specified by the "moptop.get_fits.keyword.<n>" properties in the fits property file.
	 * @param list A Vector of FitsHeaderCardImage instances. A subset of these will be passed to the C layer.
	 * @exception Exception Thrown if addFitsHeader fails.
	 * @see #addFitsHeader
	 * @see ngat.fits.FitsHeaderCardImageKeywordComparator
	 * @see ngat.fits.FitsHeaderCardImage
	 */
	protected void addISSFitsHeaderList(List list) throws Exception
	{
		FitsHeaderCardImageKeywordComparator cardImageCompareByKeyword = null;
		FitsHeaderCardImage keyCardImage = null;
		FitsHeaderCardImage valueCardImage = null;
		String keyword;
		boolean done;
		int index,listIndex,cameraCount,cLayerIndex,cameraIndex;

		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":addISSFitsHeaderList:started.");
		// sort the List into keyword order.
		cardImageCompareByKeyword = new FitsHeaderCardImageKeywordComparator();
		Collections.sort(list,cardImageCompareByKeyword);
		// iterate over keywords to copy
		index = 0;
		done = false;
		keyCardImage = new FitsHeaderCardImage();
		while(done == false)
		{
			moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":addISSFitsHeaderList:Looking for keyword index "+index+" in copy list.");
			keyword = status.getProperty("moptop.get_fits.keyword."+index);
			if(keyword != null)
			{
				// find the keyword in the list
				moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
					   ":addISSFitsHeaderList:Looking for "+keyword+" in ISS list.");
				keyCardImage.setKeyword(keyword);
				listIndex = Collections.binarySearch(list,keyCardImage,cardImageCompareByKeyword);
				if(listIndex > -1)
				{
					// we found the keyword in the GET_FITS list
					valueCardImage = (FitsHeaderCardImage)(list.get(listIndex));
					moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
						   ":addISSFitsHeaderList:Adding "+keyword+" to all C layers.");
					// Add FITS header to all cameras in all C layers
					cameraCount = status.getPropertyInteger("moptop.camera.count");
					for(int i = 0; i < cameraCount; i++)
					{
						cLayerIndex = status.getPropertyInteger("moptop.camera.c.layer."+i);
						cameraIndex = status.getPropertyInteger("moptop.camera.index."+i);
						moptop.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
							   ":addISSFitsHeaderList:Adding "+keyword+
							   " to C layer index "+cLayerIndex+" and camera index "+
							   cameraIndex+".");
						addFitsHeader(cLayerIndex,cameraIndex,keyword,
							      valueCardImage.getValue());
					}
				}
				else
				{
					// we failed to find the keyword in the GET_FITS list
					moptop.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+
						   ":addISSFitsHeaderList:Failed to find "+keyword+" in ISS list.");
				}
				// try next keyword
				index++;
			}
			else
				done = true;
		}
		moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":addISSFitsHeaderList:finished.");
	}

	/**
	 * Method to add the specified FITS header to the C layers list of FITS headers. 
	 * The C layer machine/port specification is retrieved from the 
	 * "moptop.c.hostname.&lt;cMachineIndex&gt;" and "moptop.c.port_number.&lt;cMachineIndex&gt;" properties. 
	 * An instance of FitsHeaderAddCommand is used to transmit the data.
	 * @param cMachineIndex Which moptop C layer program to send the FITS header to.
	 * @param cameraIndex Which camera we wish to add the header for.
	 * @param keyword The FITS headers keyword.
	 * @param value The FITS headers value - an object of class String,Integer,Float,Double,Boolean,Date.
	 * @exception Exception Thrown if the FitsHeaderAddCommand internally errors, or the return code indicates a
	 *            failure.
	 * @see #status
	 * @see #dateFitsFieldToString
	 * @see ngat.moptop.MoptopStatus#getProperty
	 * @see ngat.moptop.MoptopStatus#getPropertyInteger
	 * @see ngat.moptop.command.FitsHeaderAddCommand
	 * @see ngat.moptop.command.FitsHeaderAddCommand#setAddress
	 * @see ngat.moptop.command.FitsHeaderAddCommand#setPortNumber
	 * @see ngat.moptop.command.FitsHeaderAddCommand#setCommand
	 * @see ngat.moptop.command.FitsHeaderAddCommand#getParsedReplyOK
	 * @see ngat.moptop.command.FitsHeaderAddCommand#getReturnCode
	 * @see ngat.moptop.command.FitsHeaderAddCommand#getParsedReply
	 */
	protected void addFitsHeader(int cMachineIndex,int cameraIndex,String keyword,Object value) throws Exception
	{
		//FitsHeaderAddCommand addCommand = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		if(keyword == null)
		{
			throw new NullPointerException(this.getClass().getName()+":addFitsHeader:keyword was null.");
		}
		if(value == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeader:value was null for keyword:"+keyword);
		}
		//addCommand = new FitsHeaderAddCommand();
		// configure C comms
		hostname = status.getProperty("moptop.c.hostname."+cMachineIndex);
		portNumber = status.getPropertyInteger("moptop.c.port_number."+cMachineIndex);
		//addCommand.setAddress(hostname);
		//addCommand.setPortNumber(portNumber);
		// set command parameters
		if(value instanceof String)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with String value "+value+".");
			//addCommand.setCommand(cameraIndex,keyword,(String)value);
		}
		else if(value instanceof Integer)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with integer value "+
				   ((Integer)value).intValue()+".");
			//addCommand.setCommand(cameraIndex,keyword,((Integer)value).intValue());
		}
		else if(value instanceof Float)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with float value "+
				   ((Float)value).doubleValue()+".");
			//addCommand.setCommand(cameraIndex,keyword,((Float)value).doubleValue());
		}
		else if(value instanceof Double)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with double value "+
				   ((Double)value).doubleValue()+".");
		        //addCommand.setCommand(cameraIndex,keyword,((Double)value).doubleValue());
		}
		else if(value instanceof Boolean)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with boolean value "+
				   ((Boolean)value).booleanValue()+".");
			//addCommand.setCommand(cameraIndex,keyword,((Boolean)value).booleanValue());
		}
		else if(value instanceof Date)
		{
			moptop.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with date value "+
				   dateFitsFieldToString((Date)value)+".");
			//addCommand.setCommand(cameraIndex,keyword,dateFitsFieldToString((Date)value));
		}
		else
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":addFitsHeader:value had illegal class:"+
							   value.getClass().getName());
		}
		// actually send the command to the C layer
		//addCommand.sendCommand();
		// check the parsed reply
		//if(addCommand.getParsedReplyOK() == false)
		//{
		//	returnCode = addCommand.getReturnCode();
		//	errorString = addCommand.getParsedReply();
		//	moptop.log(Logging.VERBOSITY_TERSE,"addFitsHeader:Command failed with return code "+
		//		   returnCode+" and error string:"+errorString);
		//	throw new Exception(this.getClass().getName()+
		//			    ":addFitsHeader:Command failed with return code "+returnCode+
		//			    " and error string:"+errorString);
		//}
	}

	/**
	 * This routine takes a Date, and formats a string to the correct FITS format for that date and returns it.
	 * The format should be 'CCYY-MM-DDThh:mm:ss[.sss...]'.
	 * @param date The date to return a string for.
	 * @return Returns a String version of the date in the correct new FITS format.
	 */
// diddly use keyword to determine format of string
// DATE-OBS format 'CCYY-MM-DDThh:mm:ss[.sss...]'
// DATE 'CCYY-MM-DD'
// UTSTART 'HH:MM:SS.s'
// others?
	private String dateFitsFieldToString(Date date)
	{
		Calendar calendar = Calendar.getInstance();
		NumberFormat numberFormat = NumberFormat.getInstance();

		numberFormat.setMinimumIntegerDigits(2);
		calendar.setTime(date);
		return new String(calendar.get(Calendar.YEAR)+"-"+
			numberFormat.format(calendar.get(Calendar.MONTH)+1)+"-"+
			numberFormat.format(calendar.get(Calendar.DAY_OF_MONTH))+"T"+
			numberFormat.format(calendar.get(Calendar.HOUR_OF_DAY))+":"+
			numberFormat.format(calendar.get(Calendar.MINUTE))+":"+
			numberFormat.format(calendar.get(Calendar.SECOND))+"."+
			calendar.get(Calendar.MILLISECOND));
	}
}

//
// $Log$
//
