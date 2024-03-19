DEFINES =	-DSYS5  -DBSD_SELECT 

CFLAGS =	-DHAS_UNISTD  -DHAS_STDLIB -DHAS_CTYPE -DHAS_SYS_IOCTL -DHAS_SYS_WAIT -DHAS_NCURSES -DHAS_UNISTD -DHAS_STDARG -DHAS_STDLIB -DHAS_SYS_WAIT -Ofast -march=native -mtune=native -flto -fcommon -s   -DSLCT_HDR 

main :	curses

install : curses 
	@./install-sh

uninstall : clean
	@./uninstall-sh

clean :
	rm -f *.o aee xae xae_dir/*.o

all :	curses

CC = clang

OBJS = aee.o control.o format.o localize.o srch_rep.o delete.o mark.o motion.o keys.o help.o windows.o journal.o file.o

.c.o: 
	$(CC) $(DEFINES) -c $*.c $(CFLAGS)

curses :	$(OBJS)
	$(CC) -o aee $(OBJS) $(CFLAGS) $(LDFLAGS) -lncurses 

aee.o: aee.c aee.h  aee_version.h
control.o: control.c aee.h  
delete.o: delete.c aee.h  
format.o: format.c aee.h  
help.o: help.c aee.h  
journal.o: journal.c aee.h  
windows.o: windows.c aee.h  
file.o: file.c aee.h  
keys.o: keys.c aee.h  
localize.o: localize.c aee.h  
mark.o: mark.c aee.h  
motion.o: motion.c aee.h  
srch_rep.o: srch_rep.c aee.h  


