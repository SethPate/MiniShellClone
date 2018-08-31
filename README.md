# MiniShellClone
A (very) simple linux shell, based on MiniShell.

I wrote this shell with a great deal of help from Stephen Brennan: https://brennan.io/2015/01/16/write-a-shell-in-c/

This was a class exercise to practice fork(), pipe(), and processes.

The shell has several built-in commands: cd, help, date, and exit.
All other commands are sent to the next shell up.
It can be terminated with a signal sent by ctrl+c.
The shell can handle a single pipe (|) operator, but not more than that.
