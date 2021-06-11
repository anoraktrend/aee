# This is the make file for aee, "another easy editor".
#
# A file called 'make.aee' will be generated which will contain information 
# specific to the local system, such as if it is a BSD or System V based 
# version of UNIX, whether or not it has catgets, or select.  
#
# The "install" target ("make install") will copy the aee and xae binaries to 
# the /usr/local/bin directory on the local system.  The man page (aee.1) 
# will be copied into the /usr/local/man/man1 directory.
#
# The "clean" target ("make clean") will remove the 
# object files, and the aee and xae binaries.
#

main :	localaee buildaee 

all :	both

aee :	main
	exit

both :	main xae

xae :	localxae buildxae

buildaee :	
	make -f make.aee

localaee:
	@./create.mk.aee

buildxae :	
	(cd xae_dir; make -f make.xae)

localxae:
	@./create.mk.xae

install :
	@./install-sh

clean :
	rm -f *.o aee xae xae_dir/*.o

