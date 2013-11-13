package midiSwitcherConfig;

public class ConfigManager {
	public static final int NUM_CHANNELS = 8;
	
	int currentChannel = 0;
	SwitcherChannel switcherChannel[];
	
	ConfigManager()
	{
		this.switcherChannel = new SwitcherChannel[NUM_CHANNELS];		
		for(int i=0;i<ConfigManager.NUM_CHANNELS;++i)
			this.switcherChannel[i] = new SwitcherChannel();
	}
	
	public String format()
	{
		StringBuilder builder = new StringBuilder();
		for(int i=0;i<ConfigManager.NUM_CHANNELS;++i)
		{
			if(i==currentChannel)
				builder.append(">");
			else
				builder.append(" ");
			builder.append("port");
			builder.append((char)('a'+i));
			builder.append(' ');
			builder.append(switcherChannel[i].format());
			builder.append("\r\n");
		}
		return builder.toString();
	}
	
	public boolean execCommand(UserCommand command, MIDIPortManager midi)
	{
		if(command.type == UserCommand.PORT || command.type == UserCommand.SLASH)
		{
			if(command.parameter.length() == 0)
				return true;
			if(command.parameter.length() != 1)
				return false;
			char ch = command.parameter.charAt(0);
			if(ch < 'a'||ch>'h')
				return false;
			currentChannel = ch - 'a';
		}
		else if(command.prefix == UserCommand.PREFIX_ALL)
		{
			for(int i=0; i<NUM_CHANNELS; ++i)
				if(!switcherChannel[i].execCommand(command, i, midi))
					return false;
		}
		else if(!switcherChannel[currentChannel].execCommand(command, currentChannel, midi))
		{
			return false;
		}
		return true;
	}
}
