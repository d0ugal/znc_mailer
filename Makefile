push.so: mailer.cpp
	znc-buildmod mailer.cpp

clean:
	-rm -f mailer.so

install:
	znc-buildmod mailer.cpp
	mv mailer.so ~/.znc/module