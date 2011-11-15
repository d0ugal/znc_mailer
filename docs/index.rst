Welcome to ZNC Mailer's documentation!
======================================

A ZNC Module that uses the Unix mail command to send mentions and private
messages to an email address when you are not connected to ZNC.

Install
========

The easiest, and best way to install ZNC Mailer is to follow these steps;

1. `Install ZNC`_.
2. :ref:`Setup the mail command<mail_setup>`.
3. Clone the `git repository`_ with `git clone git://github.com/d0ugal/znc_mailer.git`
4. cd into the directory `cd znc_mailer` and run the command `make install` (No sudo required.)

This assumes you have a fairly standard ZNC installation, meaning that the
modules directory is located in ~/.znc/modules

Usage
=======

After you have installed, in your IRC client you will need to activate the module.
When connected to ZNC, the quickest way to do this is to send the command::

    /msg *status loadmodule mailer

After doing this, you will recieve a private message from the module requesting
an email address to use. Reply in the format "email myemail@domain.com"

After that, you should be done and you will start to recieve emails when you
are not attached to your ZNC instance.


Contents
========

.. toctree::
 :maxdepth: 1

 mail


.. _Install ZNC: http://wiki.znc.in/Installation
.. _git repository: https://github.com/d0ugal/znc_mailer