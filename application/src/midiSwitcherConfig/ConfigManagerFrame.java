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
	JTextArea textArea;
	JTextField textField;

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
    		Toolkit.getDefaultToolkit().beep();
   		textField.setText(input);
   		if(updated == midiPortManager)
   			textArea.setText(midiPortManager.format());
   		else
   			textArea.setText(configManager.format());
    }
	
	ConfigManagerFrame(String title)
	{
		super(title);
		this.configManager = new ConfigManager();
		this.midiPortManager = new MIDIPortManager();
		
		setLayout(new GridLayout());
		JPanel panel = new JPanel();
		textArea = new JTextArea();
		textField = new JTextField();
      
		Font font = new Font("Courier", Font.BOLD, 16);
		textArea.setFont(font);
		textArea.setEditable(false);
		textArea.setAutoscrolls(true);
            
		JScrollPane scrollPane = new JScrollPane(textArea); 
      
		panel.setPreferredSize(new Dimension(500,300));
		BorderLayout borderLayout = new BorderLayout();
		panel.setLayout(borderLayout);
		panel.add(scrollPane, BorderLayout.CENTER);
		panel.add(textField, BorderLayout.SOUTH);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      
		textField.addActionListener(this);
		add(panel);
		pack();
		textArea.setText(midiPortManager.format());
		textField.requestFocusInWindow();
	}
		
}
