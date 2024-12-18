OBJS=	BITMAP.OBJ CGA.OBJ DETECT.OBJ DITHER.OBJ EGA.OBJ GLOBALS.OBJ MDA.OBJ SHOW.OBJ SYSTEM.OBJ VGA.OBJ
PRG=	show.exe
CFLAGS=	-ms -0 -otexan -fpc

$(PRG): $(OBJS)
	wlink @watcom.lnk
        
.C.OBJ:
	wcc $(CFLAGS) $<

clean:
	del *.exe
	del *.obj

