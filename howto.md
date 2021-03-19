---
layout: default
title: how to
---

# Beginner's Guide to Internet Relay Chat (IRC)

For those that are new to the **Internet Relay Chat (IRC)** Protocol, learning the commands can be a daunting task. A glance through the public [RFC 1459](https://tools.ietf.org/html/rfc1459) documentation reveals a `~65` page maze of “cryptic  and strange jargon”. Most IRC clients attempt to _simply_ this maze by defining their own command structures with shorter and easier to remember aliases.  Shorter commands means fewer keystrokes, which (most assume) translates to more productivity. The question ultimately becomes, why would anyone take the time learn these IRC Protocol commands?

The answer to this lies in the _power_ of each IRC Protocol command and the _flexibility_ that that is gained. By using the IRC Protocol commands, one is able to interact in a consistent manner across all IRC services and clients. This means, no matter what half-decent client you are connecting with, you should be able to send a messages and interact with the _core_ IRC server without the need to learn custom commands for each client.

## The Basics

The following section will attempt to guide new users on the establishing a connection with an IRC server, joining additional channels and sending a few messages.  For the sake of simplicity, we will assume that _kirc_ is the intended IRC client AND has already been installed by the user (per the `README.md` instructions).

## Establishing a New Connection

In order to establish a connection with an IRC server, we need to know a few basic details about the target IRC service.

| Parameter             | Description                                                                                                                                                                                                                                                                                                                                               |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Host Address          | This is typically the IP Address or URL provided by the service provider. One of the most popular free services include the [Freenode](https://freenode.net/), which happens is the default IRC server of kirc. It’s important to note that all messages sent or received on a Freenode server are in public domain and accessible by anyone (i.e. [logbot](https://freenode.logbot.info/)). |
| Port Number           | The physical numeric port address to which the client uses to connect to the server. By default, most IRC servers use 6667. Note that this also happens to be the default port of kirc.                                                                                                                                                                   |
| Authentication Method | Some services may require you to register for access credentials before making your initial connection. In most cases, this can than be passed to the client as command arguments (e.g -u or -k). For other services, such as Freenode, a username can be registered after the initial connection.                                                        |
| Nickname              | In most cases, a nickname is required and must be passed to the IRC server before the access to credentials. Typically, it does not matter which nickname the user chooses as this can be changed later once a connection has been established.                                                                                                           |

### Example 1: connecting with registered credentials

Once the above information has been obtained, the IRC client can be launched. The following line is an example of what this might look like for a _Freenode_ IRC server with registered credentials `kircbot` (as the username) and `sesame` (as the password). 

```shell
kirc -s irc.freenode.net -p 6667 -n kircbot -u kircbot -k sesame
```

### Example 2: connecting without registered credentials

If you would like to connect to the _Freenode_ IRC server without specifying credentials (e.g. have not registered), then you can do so with the following command (remember to replace `kircbot` with a nickname of your choosing):

```shell 
kirc -n kircbot
```

*  Upon a successful connection, _many_ of lines of text should be printed to the terminal window. While it’s not important to read all of these lines, everyone should at least review the server’s __rules and policies__.
*  Optional: To close the current server connection and exit _kirc_, type `/QUIT` and press the _RETURN_ or _ENTER_ key. 

## Joining Additional Channels

A __channel__ is a named group of one or more clients which will all receive messages addressed to that channel.  A user (or client) can connect to one or many channels simultaneously.  When a message is received or sent, it is done so in accordance IRC server’s local time and parsed in the priority that which it was received. 

### Example 1: joining the #kirc channel

In the provided example previous section, when we connected to the _Freenode_ IRC server without specifying a channel to join. In this example, we will try to connect to the _#kirc_ channel, which is one that I created for new users to experiment with, practice and ask me (or other users) questions!

*   If you have not yet closed the connection from the previous section, you can try to join additional channels by specifying the `/JOIN <channel>` command. For example, to join the __#kirc__ channel on the _Freenode_ IRC server, one could type `/JOIN #kirc` and press the _RETURN_ or _ENTER_ key. 
*   Note that if this command was entered correctly, a list of all active users in that channel should be printed to the terminal window and you should start seeing messages being sent to that channel printed to your terminal window.  
 
## Sending Your First Message(s)

Now comes the “fun” part... sending your first _legitimate_ message! In order to send a message it’s important to understand the command structure.  To send a message to a specific channel, one needs to simply use the __Private Message__ or `PRIVMSG` command. This command uses the the following syntax:

```
/PRIVSMG <channel|nick> :<message>
```

Based on the syntax above, note that you can also send private messages to specific nicknames (so long as you know that user’s nickname).  

### Example 1: sending a private message to the #kirc channel

Lets say I wanted to send “Hello World!” as a private message to the __#kirc__ channel. One would simply type `/PRIVMSG #kirc :Hello World!`.

### Example 2: using _kirc_ aliases to send a private message

Remember how i said aliases were _bad_ in the Introduction? Well, who wants to type “PRIVMSG” with the channel name every time you want to send a message! Luckily, _kirc_ does contain few aliases to make life easier. It also remembers a “default” message channel, so you don’t have to type it in every time.

*   To update the current saved message channel, type `\#<channel>` and press the _RETURN_ or _ENTER_ key. Remember to replace `<channel>` with the name of the channel you would like to send private messages to by default.
*   To use the private message alias, simply type your message and press the _RETURN_ or _ENTER_ key!

## Learning More Commands

Like any new tool or language, it takes time a practice to be become proficient.  It also never hurts to ask for help. Luckily, most IRC servers have a built-in “help” function, which can be accessed with the following command:

```
\HELP
```

This should print a list of available server commands to your console.  In order to enquire on the usage of a specific command, you simply “append” the command name after _HELP_.  

For example, if i wanted to learn about the `\NAMES` Function, I could type:

```
\HELP NAMES
```

## Summary

This concludes this IRC protocol primer! As of now, you should know same of the basic IRC commands, a few _kirc_ specific aliases, as well as ways to discover more commands. 

[back]({{ site.url }})
