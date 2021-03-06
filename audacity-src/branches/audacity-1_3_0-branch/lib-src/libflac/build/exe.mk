#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#
# GNU makefile fragment for building an executable
#

ifeq ($(DARWIN_BUILD),yes)
CC          = cc
CCC         = c++
else
CC          = gcc
CCC         = g++
endif
NASM        = nasm
# LINKAGE can be forced to -static or -dynamic from invocation if desired, but it defaults to -static except on OSX
ifeq ($(DARWIN_BUILD),yes)
LINKAGE     = -dynamic
else
LINKAGE     = -static
endif
LINK        = $(CC) $(LINKAGE)
BINPATH     = ../../obj/bin
LIBPATH     = ../../obj/lib
PROGRAM     = $(BINPATH)/$(PROGRAM_NAME)

all : release

include ../../build/config.mk

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W -Winline -DFLaC__INLINE=__inline__ -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

debug   : $(ORDINALS_H) $(PROGRAM)
release : $(ORDINALS_H) $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(LINK) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o : %.cc
	$(CCC) $(CFLAGS) -c $< -o $@
%.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@
%.i : %.cc
	$(CCC) $(CFLAGS) -E $< -o $@

%.o : %.nasm
	$(NASM) -f elf -d OBJ_FORMAT_elf -i ia32/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(PROGRAM)

.PHONY : depend
depend:
	makedepend -fMakefile.lite -- $(CFLAGS) $(INCLUDES) -- *.c *.cc
