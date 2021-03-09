# kirc

kirc (KISS for IRC) is a tiny IRC client written in POSIX C99.

## Installation & Usage

Building and installing from source:

```
git clone https://github.com/mcpcpc/kirc
cd kirc
make
make install
```

### Usage

Consult `man kirc` for a full list and explanation of available `kirc` arguments.

```
kirc [-s hostname] [-p port] [-c channels] [-n nickname] [-r realname] [-u username]
     [-k password] [-a token] [-x command] [-o logfile] [-e|v|V]
```

### Command Aliases

```shell
<message>                  Send a PRIVMSG to the current channel.
@<channel|nick> <message>  Send a message to a specified channel or nick 
@@<channel|nick> <message> Send a CTCP ACTION message to a specified channel or nick 
/<command>                 Send command to IRC server (see RFC 2812 for full list).
/#<channel>                Assign new default message channel.
```

### User Input Key Bindings

* **CTRL+B** or **LEFT ARROW** moves the cursor one character to the left.
* **CTRL+F** or **RIGHT ARROW** moves the cursor one character to the right.
* **CTRL+P** or **UP ARROW** moves to previous record in the input history buffer.
* **CTRL+N** or **DOWN ARROW** move to next record in the input history buffer.
* **CTRL+E** moves the cursor to the end of the line.
* **CTRL+A** or **HOME** moves the cursor to the start of the line.
* **CTRL+W** deletes the previous word.
* **CTRL+U** deletes the entire line.
* **CTRL+K** deletes the from current character to end of line.
* **CTRL+C** Force quit kirc.
* **CTRL+D** deletes the character to the right of cursor.
* **CTRL+T** swap character at cursor with previous character.
* **CTRL+H** equivalent to backspace.

## Support Documentation

Please refer to the official for examples, troubleshooting and use cases.

* https://mcpcpc.github.io/kirc/documentation.html

## Contact

For any further questions or concerns, feel free to email me.

* michaelczigler [at] mcpcpc [dot] com.
