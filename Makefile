#makefile for fltk program
CXXFLAGS="-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti"
PROG=flume
PROG_CAPS=FLUME
DESC="Math Scratchpad"
TCEDIR=`cat /opt/.tce_dir`
BASEDIR=`pwd`

all: math_expression.cpp ${PROG}.cpp
	g++ ${CXXFLAGS} `fltk-config --cxxflags` -c ${PROG}.cpp
	g++ -c math_expression.cpp  -Os 
	gcc `fltk-config --use-images --ldflags` -lfltk_images -lm ${PROG}.o math_expression.o -o ${PROG}
	strip ${PROG}
	sudo cp ${PROG} /usr/local/bin
	sudo cp ${PROG}_help.htm /usr/share/doc/tc/
	sudo cp ${PROG}.gif /usr/share/doc/tc/
	sudo cp ${PROG}.png /usr/local/share/pixmaps/

mathexp: math_expression.cpp
	g++ math_expression.cpp  -DSTANDALONE -lm -Os -o mathexp
	
clean:
	rm *.o ${PROG}

run:
	flume &
	
oldpackage:
	./build-tcz_for_tc_2_10.sh

package:
	./build-tcz.sh ${PROG} ${PROG_CAPS} ${BASEDIR} ${DESC}

deploy: package
	echo "TCE DIR is ${TCEDIR}"
	cp ${BASEDIR}/${PROG}.tcz ${TCEDIR}/optional

tarball:
	tar -czf ${PROG}_src.tar.gz flume.cpp math_expression.cpp math_expression.h flume.png flume_help.htm build-tcz.sh flume.gif flume.tcz.info Makefile
	
unpack:
	tar -xzf ${PROG}_src.tar.gz

