package midiSwitcherConfig;

import java.util.HashMap;
import java.util.Map.Entry;


public class UserCommand {
	public static final int NONE = 0;
	public static final int PORT = 1;
	public static final int SLASH = 2;
	public static final int CHANNEL = 100; 
	public static final int NOTE = 101;
	public static final int DURATION_MODULATOR = 102;
	public static final int DURATION = 103;
	public static final int DUTY_MODULATOR = 104;
	public static final int DUTY = 105;
	public static final int INVERT = 106;
	public static final int TRANSMIT_ALL = 107;
	public static final int COMMIT = 108;
	public static final int TRIGGER = 109;
	public static final int DEVICE = 200;

	public static final int CHAN_COMMAND_START = 100;
	public static final int DEVICE_COMMAND_START = 200;

	public static final int PREFIX_NONE = 0;
	public static final int PREFIX_ALL = 1;
	
	public static final String TAG_SLASH = "/";
	public static final String TAG_PORT = "port";
	public static final String TAG_CHANNEL = "ch";
	public static final String TAG_NOTE = "@";
	public static final String TAG_DURATION = "dur";
	public static final String TAG_DURATION_MODULATOR = "dur~";
	public static final String TAG_DUTY = "duty";
	public static final String TAG_DUTY_MODULATOR = "duty~";
	public static final String TAG_INVERT = "inv";	
	public static final String TAG_DEVICE = "dev";
	public static final String TAG_TRANSMIT_ALL = "!";
	public static final String TAG_COMMIT = "$";
	public static final String TAG_TRIGGER = ".";

	public static final String TAG_PREFIX_ALL= "*";
	
	
	
	static HashMap<Integer, String> keywords;
	static {
		UserCommand.keywords  = new HashMap<Integer, String>();
		UserCommand.keywords.put(DEVICE, TAG_DEVICE);
		UserCommand.keywords.put(PORT, TAG_PORT);
		UserCommand.keywords.put(SLASH, TAG_SLASH);
		UserCommand.keywords.put(TRANSMIT_ALL, TAG_TRANSMIT_ALL);
		UserCommand.keywords.put(CHANNEL, TAG_CHANNEL);
		UserCommand.keywords.put(NOTE, TAG_NOTE);
		UserCommand.keywords.put(DURATION_MODULATOR, TAG_DURATION_MODULATOR);
		UserCommand.keywords.put(DURATION, TAG_DURATION);
		UserCommand.keywords.put(DUTY_MODULATOR, TAG_DUTY_MODULATOR);
		UserCommand.keywords.put(DUTY, TAG_DUTY);
		UserCommand.keywords.put(INVERT, TAG_INVERT);
		UserCommand.keywords.put(TRIGGER, TAG_TRIGGER);
	}
	
	int type = NONE;
	int prefix = PREFIX_NONE;
	String parameter;
	String remainingInput;		
	UserCommand(String input)
	{
		input = input.trim();
		if(input.startsWith(">"))
			input = input.substring(1);//ignore ">" at start
		if(input.startsWith(TAG_PREFIX_ALL))
		{
			input = input.substring(TAG_PREFIX_ALL.length());
			this.prefix = PREFIX_ALL;
		}
		Entry<Integer,String> pos = null;
		for(Entry<Integer,String> i : UserCommand.keywords.entrySet())
		{
			if(input.startsWith(i.getValue()))
			{
				pos = i;
				break;
			}
		}
		if(pos != null)
		{
			this.type = pos.getKey();
			int spacePos = input.indexOf(' ');
			int paramPos = pos.getValue().length();
			if(spacePos < 0)
			{
				this.parameter = input.substring(paramPos);
				this.remainingInput = "";
			}
			else
			{
				this.parameter = input.substring(paramPos, spacePos);
				this.remainingInput = input.substring(spacePos + 1);
			}
		}
	}
	
	public static int getIntParam(String param, int min, int max)
	{
		int value = Integer.parseInt(param);
		if(value < min) 
			return min;
		if(value > max) 
			return max;
		return value;
	}
	
};
