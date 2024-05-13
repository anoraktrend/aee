DEFINES =	-DSYS5 -DBSD_SELECT 

CFLAGS =	-DHAS_UNISTD -fuse-ld=lld -DHAS_STDLIB -DHAS_CTYPE -DHAS_SYS_IOCTL -DHAS_SYS_WAIT -DHAS_UNISTD -DHAS_STDARG -DHAS_STDLIB -DHAS_SYS_WAIT -Ofast -march=native -mtune=native -flto -fcommon -s -DSLCT_HDR 

main :	ncurses

install : main 
	@./install-sh

uninstall : clean
	@./uninstall-sh

clean :
	rm -f *.o aee

all :	ncurses

CC = clang

OBJS = aee.o control.o format.o localize.o srch_rep.o delete.o mark.o motion.o keys.o help.o windows.o journal.o file.o

.c.o: 
	$(CC) $(DEFINES) -c $*.c $(CFLAGS)

ncurses :	$(OBJS)
	$(CC) -o aee $(OBJS) $(CFLAGS) $(LDFLAGS) -DNCURSE -DCURSES -DHAS_NCURSES -lncursesw 

aee.o: aee.c aee.h new_curse.h  aee_version.h
control.o: control.c  aee.h new_curse.h  
delete.o: delete.c  aee.h new_curse.h  
format.o: format.c  aee.h new_curse.h  
help.o: help.c  aee.h new_curse.h  
journal.o: journal.c  aee.h new_curse.h  
windows.o: windows.c  aee.h new_curse.h  
file.o: file.c  aee.h new_curse.h  
keys.o: keys.c  aee.h new_curse.h  
localize.o: localize.c  aee.h new_curse.h  
mark.o: mark.c aee.h new_curse.h  
motion.o: motion.c  aee.h new_curse.h  
srch_rep.o: srch_rep.c aee.h new_curse.h
new_curse.o: new_curse.c aee.h new_curse.h
