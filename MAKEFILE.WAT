OBJS=	CGA.OBJ CPLUS.OBJ DETECT.OBJ DITHER.OBJ EGA.OBJ GLOBALS.OBJ MDA.OBJ PCX.OBJ SHOW.OBJ SYSTEM.OBJ VGA.OBJ WCCOMPAT.OBJ
PRG=	show.exe
CFLAGS=	-q -ms -0 -otexan -fpc

$(PRG): $(OBJS)
	wlink @watcom.lnk
        
.C.OBJ:
	@set DOS4G=quiet
	wcc $(CFLAGS) $<

clean:
	del *.exe
	del *.obj

