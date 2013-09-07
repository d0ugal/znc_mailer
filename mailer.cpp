/**
 * ZNC Mailer Module
 *
 * A ZNC Module that uses the Unix mail command to send mentions and private
 * messages to an email address when you are not connected to ZNC.
 *
 * Copyright (c) 2011 Dougal Matthews
 * Licensed under the MIT license
 * Edited by Nox (2013)
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

// Convert an int into a string.
CString intToString (int value) {
	std::stringstream ss;
	ss << value;
	CString sValue = ss.str();
	return sValue;
}

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

        // Recover config vars.
        NotificationEmail = GetNV("email");
        NotificationSubject = GetNV("subject");
        unsigned int interval = atoi(GetNV("interval").c_str());
        MaxNotifications = atoi(GetNV("maxNotifications").c_str());

        if (DebugMode) {
            PutModule("NV: email: " + NotificationEmail);
            PutModule("NV: subject: " + NotificationSubject);
            PutModule("NV: interval: " + intToString(interval));
            PutModule("NV: maxNotifications: " + intToString(MaxNotifications));
        }

        // Set to default if vars are missing.
        if (interval <= 0) {
            // Default to 20 mins, debug to 60 seconds.
            interval = 1200;
            if (DebugMode){
                interval = 60;
            }
            SetNV("interval", "1200");
        }
        if (MaxNotifications <= 0) {
            // Default to 50.
            MaxNotifications = 50;
            SetNV("maxNotifications", "50");
        }
        

        AddTimer(new CMailerTimer(this, interval, 0, "Mail", "Send emails every "+intToString(interval)+" mins"));

        if (NotificationSubject == "") {
            NotificationSubject = "IRC Notification";
            SetNV("subject", "IRC Notification");
            PutModule("Default subject : 'IRC Notification' (change whith 'subject <subject>')");
        }

        if (NotificationEmail == "") {
            PutModule("Please tell me what email address you want notifications to be sent to with 'email <address>'");
        }

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

            SetNV("email", NotificationEmail);
            PutModule("email address set to : "+ NotificationEmail);

        } else if (action == "subject"){

            if (token_count < 2)
            {
                PutModule("Subject: " + NotificationSubject);
                PutModule("Usage: subject <email subject>");
                return;
            }

            NotificationSubject = tokens[1];
            for (int i = 2; i < token_count; i++) {
                NotificationSubject += " " + tokens[i];
            }
            SetNV("subject", NotificationSubject);
            PutModule("email subject set to : "+ NotificationSubject);

        } else if (action == "interval") {

            if (token_count < 2)
            {
                PutModule("Interval: " + GetNV("interval"));
                PutModule("Usage: interval <seconds>");
                return;
            }

            SetNV("interval", tokens[1]);
            PutModule("interval set to : "+ tokens[1] + " , will be effective after reloading mailer : '/msg *status reloadmod mailer'");
            
        } else if (action == "notifmax") {

            if (token_count < 2)
            {
                PutModule("maxNotifications: " + intToString(MaxNotifications));
                PutModule("Usage: notifmax <number>");
                return;
            }

            MaxNotifications = atoi(tokens[1].c_str());

            SetNV("maxNotifications", tokens[1]);
            PutModule("maxNotifications set to : "+ tokens[1]);
            
        } else if (action == "help") {

                PutModule("Commands :");
                PutModule("  email <email address>");
                PutModule("       set the email to sent notifications to.");
                PutModule("  subject <email subject>");
                PutModule("       set the subject of the notification, default IRC Notification.");
                PutModule("  interval <seconds>");
                PutModule("       set the time between the first message and the mail notification, default 1200 (20 minutes).");
                PutModule("       Reload module required ('/msg *status reloadmod mailer')");
                PutModule("  notifmax <number>");
                PutModule("       set the number of notification in one mail, default 50.");
                PutModule(" ");
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
