# kirc

kirc (KISS for IRC) is a tiny IRC client written in POSIX C99.

## Installation

Building and installing from source:

	git clone https://github.com/mcpcpc/kirc
	cd kirc
	make
	make install

## Usage

Consult `man kirc` for a full list and explanation of available arguments.

    kirc [-s hostname] [-p port] [-c channels] [-n nickname] [-r realname] [-a auth]
         [-u username] [-k password] [-a token] [-o logfile] [-D directory] [-4|6|d|e|x|v|V]

## DCC
	DCC transfers are only accepted if the -d flag is passed to kirc.
 	If -d is passed to kirc, DCC transfers are always accpeted without user interaction
  	and downloaded to the configured directory.
	If no directory is set with the -D option, files are downloaded to the current directory.

 ## DCC SEND
	DCC SEND is only enabled if the -d flag is passed to kirc.
	You can send files via DCC SEND using the folowing command:
	/dcc <target nick> <path to file> <internal ip> <external ip> <port>
	If you are sending files to clients outside of your lan, you must
	make sure that the chosen port is open and forwarded to
	the sending machine's internal address.
	Thus is usually done from your router's settings.
	For example:
	/dcc dcc_client /home/user/upload/file.txt 192.168.1.100 12.34.56.78 10000
	This command would send the file located at /home/user/upload/file.txt to the nick
	dcc_client, assuming the sending machine's local ip is 192.168.1.100, and
 	the sending machine's external ip is 12.34.56.78, and that the port 10000
  	is open and forwarded to the sending machine's local address.

## Command Aliases

    <message>                   send PRIVMSG to the current channel.
    @<channel|nick> <message>   send PRIVMSG to a specified channel or nick.
    @@<channel|nick> <message>  send CTCP ACTION message to a specified channel
                                or nick (if no channel or nick is specified, the
                                message will be sent to the default channel).
    /<command>                  send command to the IRC server (see RFC 2812).
    /#<channel>                 assign new default message channel.

## Key Bindings

    CTRL+B or LEFT ARROW        move the cursor one character to the left.
    CTRL+F or RIGHT ARROW       move the cursor one character to the right.
    CTRL+P or UP ARROW          move to previous record in the input history buffer.
    CTRL+N or DOWN ARROW        move to next record in the input history buffer.
    CTRL+E                      move the cursor to the end of the line.
    CTRL+A or HOME              move the cursor to the start of the line.
    CTRL+W                      delete the previous word.
    CTRL+U                      delete the entire line.
    CTRL+K                      delete the from current character to end of line.
    CTRL+D                      delete the character to the right of cursor.
    CTRL+C                      force quit kirc.
    CTRL+T                      swap character at cursor with previous character.
    CTRL+H                      equivalent to backspace.

## Support Documentation

Please refer to the [official homepage](http://kirc.io/docs.html) for examples, 
troubleshooting and use cases.

## Contact

For any further questions or concerns, feel free to send me an 
[email](michaelczigler[at]mcpcpc[dot]com).
