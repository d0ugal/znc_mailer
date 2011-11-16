push.so: mailer.cpp
	znc-buildmod mailer.cpp

clean:
	-rm -f mailer.so

install:
	make clean
	znc-buildmod mailer.cpp
	cp -i mailer.so ~/.znc/modules