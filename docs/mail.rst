.. _mail_setup:

Setting up the mail command
======================================

You may need to setup the mail command on your system. A quick way to check is
by running ths following command to see if it works::

    echo "Testing email" | mail -s "Subject" your_email@example.com

If you get the email, then you can probably skip the rest of this page.


Setting up with Gmail/IMAP
-------------------------------

This quick guide will make a number of assumptions. You are on Ubuntu and happy
to use postfix. It's fairly straight forward::

    sudo apt-get install postfix
    sudo cp /usr/share/postfix/main.cf.debian /etc/postfix/main.cf

With your favourite editor, open up `/etc/postfix/main.cf` as sudo. And append
the following lines to the bottom of the document::

    relayhost = [smtp.gmail.com]:587
    smtp_sasl_auth_enable = yes
    smtp_sasl_password_maps = hash:/etc/postfix/sasl_passwd
    smtp_sasl_security_options = noanonymous
    smtp_tls_CAfile = /etc/postfix/cacert.pem
    smtp_use_tls = yes

Save the changes and then create a new file `/etc/postfix/sasl_passwd` and enter
the following line into it::

    [smtp.gmail.com]:587    <username>@gmail.com:<password>

Save and quit and return to running these commands::

    sudo chmod 400 /etc/postfix/sasl_passwd
    sudo postmap /etc/postfix/sasl_passwd
    sudo postalias hash:/etc/aliases
    cat /etc/ssl/certs/Equifax_Secure_CA.pem >> /etc/postfix/cacert.pem
    /etc/init.d/postfix restart
    apt-get install mailutils

Finally, you should now be done. To test, try one more time to send an email
with the command shown at the top of this page::

    echo "Testing email" | mail -s "Subject" your_email@example.com
