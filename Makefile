#CC=gcc
#LD=gcc
SED=sed
STRIP=strip

PREFIX?=/usr/local
DEST=$(PREFIX)/bin
MAN=$(PREFIX)/share/man/man1

CFLAGS?=-Wall -Werror -O2
LDFLAGS?=
INCLUDE=-I/usr/include/sensors -I/usr/local/include/sensors
LIBS=-L/usr/X11R6/lib -L/usr/local/lib -lX11 -lXext -lXpm -lsensors

XPM=xpm/celcius_on.o xpm/celcius_off.o xpm/fahrenheit_on.o \
	xpm/fahrenheit_off.o xpm/kelvin_on.o xpm/kelvin_off.o xpm/parts.o
OBJS=main.o dockapp.o temp.o $(XPM)
	
TARGET=wmtemp
MANPAGE=wmtemp.1x

all: $(TARGET)

%.c: %.xpm
	echo "#include \"xpm.h\"" >$@
	$(SED) -e "s/^static //" <$< >>$@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f *.o xpm/*.o xpm/*.c core $(TARGET)

install:
	cp $(TARGET) $(DEST)/$(TARGET)
#	$(STRIP) $(DEST)/$(TARGET)
	chmod 755 $(DEST)/$(TARGET)
#	cp $(MANPAGE) $(MAN)/$(MANPAGE)
#	gzip -9 $(MAN)/$(MANPAGE)
#	chmod 644 $(MAN)/$(MANPAGE)
	
main.o: dockapp.h temp.h xpm/xpm.h
dockapp.o: dockapp.h
temp.o: temp.h
xpm/xpm.h: $(XPM)
