
 # BSD 3-Clause License
 # 
 # Copyright (c) 2025, Omniscio
 # 
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:
 # 
 # 1. Redistributions of source code must retain the above copyright notice, this
 #    list of conditions and the following disclaimer.
 # 
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 #    this list of conditions and the following disclaimer in the documentation
 #    and/or other materials provided with the distribution.
 # 
 # 3. Neither the name of the copyright holder nor the names of its
 #    contributors may be used to endorse or promote products derived from
 #    this software without specific prior written permission.
 # 
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 # DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 # FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 # DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 # SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 # CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 # OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 # 
 # 

BUILDDIR=/tmp/morserator/bin
CC=gcc -g -Wall
LIBS=-lm -lSDL2 -lSDL2_ttf
MKDIR=mkdir -p
#OBJS=complex.o config.o fft.o morse.o waterfall.o
OBJS=complex.o config.o db.o morse.o waterfall.o
RM=rm -rf
TARGET=$(BUILDDIR)/morserator

$(TARGET): main.c ui.c $(OBJS)
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(TARGET) ui.c main.c $(OBJS) $(LIBS)

complex.o: complex.c complex.h
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/test -DTEST complex.c $(LIBS)
	$(BUILDDIR)/test
	$(CC) -c complex.c

config.o: config.c config.h
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/test -DTEST config.c $(LIBS)
	$(BUILDDIR)/test
	$(CC) -c config.c

db.o: db.c db.h
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/test -DTEST db.c $(LIBS)
	$(BUILDDIR)/test
	$(CC) -c db.c

#fft.o: fft.c fft.h complex.o
#	$(MKDIR) $(BUILDDIR)
#	$(CC) -o $(BUILDDIR)/test -DTEST fft.c complex.o $(LIBS)
#	$(BUILDDIR)/test
#	$(CC) -c fft.c

morse.o: morse.c morse.h complex.o
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/test -DTEST morse.c complex.o db.o $(LIBS)
	$(BUILDDIR)/test
	$(CC) -c morse.c
#	$(CC) -c morse.c -DMORSE_DEBUG_ONOFF

#sound.o: sound.c sound.h
#	$(MKDIR) $(BUILDDIR)
#	$(CC) -o $(BUILDDIR)/test -DTEST sound.c $(LIBS)
#	$(BUILDDIR)/test
#	$(CC) -c sound.c

waterfall.o: waterfall.c waterfall.h complex.o morse.o db.o
	$(MKDIR) $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/test -DTEST waterfall.c complex.o morse.o db.o $(LIBS)
	$(BUILDDIR)/test
	$(CC) -o $(BUILDDIR)/test -DTEST -DWATERFALL_COMPLEX_INPUT waterfall.c complex.o morse.o db.o $(LIBS)
	$(BUILDDIR)/test
	$(CC) -c waterfall.c

install: $(TARGET)
	cp $(TARGET) /usr/local/bin
	chmod 755 /usr/local/bin/morserator

clean:
	$(RM) -Rf $(BUILDDIR) *.o $(TARGET)
