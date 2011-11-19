/**
 * ZNC Mailer Module
 *
 * A ZNC Module that uses the Unix mail command to send mentions and private
 * messages to an email address when you are not connected to ZNC.
 *
 * Copyright (c) 2011 Dougal Matthews
 * Licensed under the MIT license
 */

#include <Chan.h>
#include <IRCSock.h>
#include <list>
#include <main.h>
#include <Modules.h>
#include <Nick.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <User.h>


// Forward de
class CMailerTimer: public CTimer {
public:

    CMailerTimer(CModule* pModule, unsigned int uInterval, unsigned int uCycles, const CString& sLabel, const CString& sDescription) : CTimer(pModule, uInterval, uCycles, sLabel, sDescription) {}
    virtual ~CMailerTimer() {}

protected:
    virtual void RunJob();

};


class CMailer : public CModule {

protected:
    CUser *ConnectedUser;
    list<CString> MessagesList;
    CString NotificationSubject;
    CString NotificationEmail;
    unsigned int MaxNotifications;
    bool DebugMode;

public:

    MODCONSTRUCTOR(CMailer) {
        ConnectedUser = GetUser();
        DebugMode = false;
        NotificationSubject = "IRC Notification";
    }

    virtual ~CMailer() {}

    void DebugPrint(string sMessage){
        if (DebugMode){
            PutModule(sMessage);
        }
    }

    virtual bool OnLoad(const CString& sArgs, CString& sErrorMsg) {
        /*
         * On module load, check for debug being passed in as an arguement.
         *
         * Then setup the timer for batch emails and prompt the user for their
         * email address.
         */

        VCString tokens;
        int token_count = sArgs.Split(" ", tokens, false);

        if (token_count >= 1){

            CString action = tokens[0].AsLower();

            if (action == "debug"){
                PutModule("DEBUG ON");
                DebugMode = true;
            }

        }

        // Default to 20 mins, debug to 60 seconds.
        unsigned int interval = 1200;
        if (DebugMode){
            interval = 60;
        }

        AddTimer(new CMailerTimer(this, interval, 0, "Mail", "Send emails every 20 mins"));
        MaxNotifications = 50;

        PutModule("Please tell me what email address you want notifications to be sent to with 'email <address>'");

        return true;

    }

    virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {

        Handle(Nick, Channel.GetName(), sMessage);

        return CONTINUE;
    }

    virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage) {

        Handle(Nick, "PRIVATE", sMessage);

        return CONTINUE;
    }

    void OnModCommand(const CString& command)
    {
        VCString tokens;
        int token_count = command.Split(" ", tokens, false);

        if (token_count < 1)
        {
            return;
        }

        CString action = tokens[0].AsLower();

        if (action == "email"){

            if (token_count < 2)
            {
                PutModule("Email: " + NotificationEmail);
                PutModule("Usage: email <email address>");
                return;
            }

            NotificationEmail = tokens[1].AsLower();

            PutModule("email address set");

        } else if (action == "subject"){

            if (token_count < 2)
            {
                PutModule("Subject: " + NotificationSubject);
                PutModule("Usage: subject <email subject>");
                return;
            }

            NotificationSubject = tokens[1].AsLower();

            PutModule("email subject set.");

        } else if (action == "help") {

                PutModule("View the documentation at http://znc-mailer.readthedocs.org/");

        } else {

            if (!DebugMode){
                PutModule("Error: invalid command, try `help`");
            }

        }

        DebugCommands(command);

    }

    void DebugCommands(const CString& command){

        if (!DebugMode){
            return;
        }

        VCString tokens;
        int token_count = command.Split(" ", tokens, false);

        if (token_count < 1)
        {
            return;
        }

        CString action = tokens[0].AsLower();

        if (action == "testemail"){

            CString message;
            message = "This is a testing email.";
            Send(message);

        } else if (action == "testfull"){

            CString message = "Testing 1";
            MessagesList.push_front(message);
            CString message2 = "Testing 2";
            MessagesList.push_front(message2);
            CString message3 = "Testing 3";
            MessagesList.push_front(message3);

            BatchSend();

        } else if (action == "testshowqueue"){

            list<CString>::iterator it;

            CString message;

            for ( it=MessagesList.begin() ; it != MessagesList.end(); it++ ){
                DebugPrint(*it);
            }

        } else if (action == "testbatchsend"){

            BatchSend();

        }

    }

    void Handle(CNick& Nick, const CString& location, CString& sMessage){
        /*
         * Method for handling all incoming messages that we are considering to
         * email to the user. All private messages and all of those that
         * SendNotification returns true for trigger an email.
         */

        if (SendNotification(Nick, sMessage) || location == "PRIVATE"){

            // Build up a string containing the current time.
            CString string_time;
            time_t rawtime;
            struct tm * timeinfo;
            char buffer [80];

            time ( &rawtime );
            timeinfo = localtime ( &rawtime );

            strftime (buffer,80,"(%x) %X",timeinfo);
            string_time =  buffer;

            // Finally construct the message as it will be added to the email.
            // and add it to a list (read queue) of messages to be sent.
            CString message = string_time + ": <" + location + ":" + Nick.GetNick() + "> " + sMessage + "\n";
            MessagesList.push_back(message);

            DebugPrint("Added message...");
            DebugPrint(message);

            // Finally, check to see if we have the max number, and if we do,
            // pop a message off the front to delete the oldest.
            if (MessagesList.size() > MaxNotifications){
                MessagesList.pop_front();
            }

        }
    }

    bool SendNotification(CNick& Nick, CString& sMessage){
        /*
         * Being passed in the nick name and the message, return true if an
         * email notification should be sent to the user or not. At the moment
         * this is a simple test that checks for a case insenstive username
         * match which is a bit flawed as 1d0ugal1 still matches d0ugal.
         */

        // Don't send notifications if DebugMode is off and the ConnectedUser is not attached.
        if (!DebugMode && ConnectedUser->IsUserAttached()){
            return false;
        }

        CString UserNick = ConnectedUser->GetNick().AsLower();

        size_t found = sMessage.AsLower().find(UserNick);

        if (found!=string::npos){
            return true;
        } else {
            return false;
        }

    }

    void BatchSend(){
        /*
         * Job to send all of the email messages in the list queue.
         */

        DebugPrint("Preparing to batch send");

        if (MessagesList.size() <= 0){
            DebugPrint("No messages");
            return;
        }

        if (!DebugMode && ConnectedUser->IsUserAttached()){
            // There is a case, that if the user recieves a message, then
            // conencts, there are some queue items waiting to be sent. Because
            // we jump out early they are never cleared until they are sent
            // when the user disconnects.
            MessagesList.clear();
            return;
        }

        list<CString>::iterator it;

        CString message;

        for ( it=MessagesList.begin() ; it != MessagesList.end(); it++ ){
            message = message + *it;
        }

        Send(message);

        MessagesList.clear();

    }

    void Send(CString sMessage){
        /*
         * Wrapper to pipe the email body to the mail command. Fairly naieve
         * about its purpose and will fail if it doesn't have a destination
         * email or can't open the mail command (i.e. it isn't installed)
         */

        DebugPrint("Preparing to send email.");

        if (NotificationEmail == ""){
            PutModule("Unable to send email, no address set.");
            return;
        }

        CString command;
        command = "mail -s '" + NotificationSubject + "' " + NotificationEmail;

        FILE *email= popen(command.c_str(), "w");

        if (!email){
            PutModule("Problems with mail command.");
            return;
        }

        DebugPrint("Opened mail command. About to write message to it.");

        fprintf(email, "%s", (char*) sMessage.c_str());
        pclose(email);

        if (DebugMode){
            DebugPrint(command);
        }
        DebugPrint(sMessage);
        DebugPrint("...Sent");

    }

};

void CMailerTimer::RunJob(){
    /*
     * Simple call to trigger the BatchSend method periodically.
     */

    CMailer *p = (CMailer *)m_pModule;

    p->BatchSend();

}

MODULEDEFS(CMailer, "To be used to send mentions as an email")
