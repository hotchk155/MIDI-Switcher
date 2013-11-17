package MidiSwitcherConfig;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.Toolkit;
import java.awt.event.ActionListener;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

@SuppressWarnings("serial")
public class ConfigManagerFrame extends JFrame 
	implements ActionListener
{
	MIDIPortManager midiPortManager;
	ConfigManager configManager;
	SequenceManager sequenceManager;
	JTextArea textArea;
	JTextField textField;

	ConfigManagerFrame(String title)
	{
		super(title);
		this.configManager = new ConfigManager();
		this.midiPortManager = new MIDIPortManager();
		this.sequenceManager = new SequenceManager();
		
		setLayout(new GridLayout());
		JPanel panel = new JPanel();
		textArea = new JTextArea();
		textField = new JTextField();
      
		Font font = new Font("Courier", Font.BOLD, 16);
		textArea.setFont(font);
		textArea.setEditable(false);
		textArea.setAutoscrolls(true);
            
		JScrollPane scrollPane = new JScrollPane(textArea); 
      
		panel.setPreferredSize(new Dimension(500,500));
		BorderLayout borderLayout = new BorderLayout();
		panel.setLayout(borderLayout);
		panel.add(scrollPane, BorderLayout.CENTER);
		panel.add(textField, BorderLayout.SOUTH);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      
		textField.addActionListener(this);
		add(panel);
		pack();
		textArea.setText(midiPortManager.toString());
		textField.requestFocusInWindow();
	}
	
    public void actionPerformed(java.awt.event.ActionEvent e) 
    {
    	String input = textField.getText();    	
		input = input.toLowerCase().trim();
		Object updated = null;
		while(input.length() > 0)
		{
			UserCommand command = new UserCommand(input);
			if(command.type == UserCommand.NONE)
				break;
			if(command.type >= UserCommand.SEQ_COMMAND_START)
			{
				updated = sequenceManager;
				if(!sequenceManager.execCommand(command, midiPortManager))
					break;
			}
			else
			if(command.type >= UserCommand.DEVICE_COMMAND_START)
			{
				updated = midiPortManager;
				if(!midiPortManager.execCommand(command))
					break;
			}
			else 
			{
				updated = configManager;
				if(!configManager.execCommand(command, midiPortManager))
					break;
			}
			input = command.remainingInput;
		}		
		if(input.length() > 0)
		{
    		Toolkit.getDefaultToolkit().beep();
		}
		else
		{
    		textArea.setText(updated.toString());
		}
		textField.setText(input);
    }		
}
