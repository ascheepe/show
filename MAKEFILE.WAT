OBJS=	BITMAP.OBJ CGA.OBJ COLOR.OBJ DETECT.OBJ DITHER.OBJ EGA.OBJ MDA.OBJ SHOW.OBJ SYSTEM.OBJ VGA.OBJ
PRG=	show.exe
CFLAGS=	-mc -0 -otexan -fpc

$(PRG): $(OBJS)
	wlink @watcom.lnk
        
.C.OBJ:
	wcc $(CFLAGS) $<

clean:
	del *.exe
	del *.obj

