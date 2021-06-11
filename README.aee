Legal stuff:

          THIS MATERIAL IS PROVIDED "AS IS".  THERE ARE NO WARRANTIES OF 
          ANY KIND WITH REGARD TO THIS MATERIAL, INCLUDING, BUT NOT 
          LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
          FITNESS FOR A PARTICULAR PURPOSE.  Neither Hewlett-Packard nor 
          Hugh Mahon shall be liable for errors contained herein, nor for 
          incidental or consequential damages in connection with the 
          furnishing, performance or use of this material.  Neither 
          Hewlett-Packard nor Hugh Mahon assumes any responsibility for 
          the use or reliability of this software or documentation.  This 
          software and documentation is totally UNSUPPORTED.  There is no 
          support contract available.  Hewlett-Packard has done NO 
          Quality Assurance on ANY of the program or documentation.  You 
          may find the quality of the materials inferior to supported 
          materials. 

      This software may be distributed under the terms of Larry Wall's 
      Artistic license, a copy of which is included in this distribution. 

      This notice must be included with this software and any 
      derivatives. 

      Any modifications to this software by anyone but the original author 
      must be so noted. 


Building the software:

	The editor 'aee' may be built on most UNIX systems by simply 
        entering 'make' at the shell prompt in the directory where 
        the source is located.  The X-Windows version, 'xae', may be 
        built using the command 'make xae', and both aee and xae may 
        be built using the single command:
        
        	make both
        
        In certain situations, the scripts which do the setup for 
        the build (normally hidden from the user) may not be able to 
        find the information necessary to build the code.  If this 
        happens, a message to that effect is provided instructing 
        the user to try manual steps to build the code.  In this 
        case a certain level of knowledge on the part of the user is 
        expected.  Most of the needed information can be deduced 
        from the files 'create.mk.aee' and 'create.mk.xae'.
        
General information:

The editor 'aee' (another easy editor) is intended to be a simple, 
easy to use terminal-based screen oriented editor that requires no 
instruction to use.  The intended audience for aee ranges from 
people who are new to computers to experienced software developers.

aee's simplified interface is highlighted by the use of pop-up menus 
which make it possible for users to carry out tasks without the need 
to remember commands.  An information window at the top of the 
screen shows the user the operations available with control-keys. 

aee allows users to use full eight-bit characters.  If the host 
system has the capabilities, aee can use message catalogs, which 
would allow users to translate the message catalog into other 
languages which use eight-bit characters.  See the file 
aee.i18n.guide for more details. 

aee relies on the virtual memory abilities of the platform it is 
running on and does not have its own memory management capabilities. 

For a text editor to be easy to use requires a certain set of 
abilities.  In order for aee to work, a terminal must have the 
ability to position the cursor on the screen, and should have arrow 
keys that send unique sequences (multiple characters, the first 
character is an "escape", octal code '\033').  All of this 
information needs to be in a database called "terminfo" (System V 
implementations) or "termcap" (usually used for BSD systems).  In 
case the arrow keys do not transmit unique sequences, motion 
operations are mapped to control keys as well, but this at least 
partially defeats the purpose.  The curses package is used to handle 
the I/O which deals with the terminal's capabilities. 

While aee is based on curses, I have included here the source code 
to new_curse, a subset of curses developed for use with aee.  
'curses' often  will have a defect that reduces the usefulness of 
the editor relying upon it.  

The file new_curse.c contains a subset of 'curses', a package for 
applications to use to handle screen output.  Unfortunately, curses 
varies from system to system, so I developed new_curse to provide 
consistent behavior across systems.  It works on both SystemV and 
BSD systems, and while it can sometimes be slower than other curses 
packages, it will get the information on the screen painted 
correctly more often than vendor supplied curses.  Unless problems 
occur during the building of aee, it is recommended that you use 
new_curse rather than the curses supplied with your system. 

If you experience problems with data being displayed improperly, 
check your terminal configuration, especially if you're using a 
terminal emulator, and make sure that you are using the right 
terminfo entry before rummaging through code.  Terminfo entries 
often contain inaccuracies, or incomplete information, or may not 
totally match the terminal or emulator the terminal information is 
being used with.  Complaints that aee isn't working quite right 
often end up being something else (like the terminal emulator being 
used).  

aee, new_curse, and Xcurse were developed using K&R C (also known as 
"classic C"), but they can also be compiled with ANSI C.  You should 
be able to build aee by simply typing "make".  A make file which 
takes into account the characteristics of your system will be 
created, and then aee will be built.  If there are problems 
encountered, you will be notified about them. 

aee is the result of conflicting design goals.  While I know that it 
solves the problems of some users, I also have no doubt that some 
will decry its lack of more features.  I will settle for knowing 
that aee does fulfill the needs of a large number of users.  The 
goals of aee are:

        1. To be so easy to use as to require no instruction.
        2. To have enough functionality to be useful to a large number of 
           people.

aee is a superset of 'ee', a simplified text editor that is (as of 
this writing) available with various free UNIX distributions (Linux 
and FreeBSD). 

Hugh Mahon              |___|     
h_mahon@fc.hp.com       |   |     
                            |\  /|
                            | \/ |

