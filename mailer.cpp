/**
 * ZNC Mailer Module
 *
 * A ZNC Module that uses the Unix mail command to send mentions and private
 * messages to an email address when you are not connected to ZNC.
 *
 * Copyright (c) 2011 Dougal Matthews
 * Licensed under the MIT license
 */

#include <main.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <list>

#include <Modules.h>
#include <IRCSock.h>
#include <User.h>
#include <Nick.h>
#include <Chan.h>


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

        AddTimer(new CMailerTimer(this, 1200, 0, "Mail", "Send emails every 20 mins."));
        MaxNotifications = 50;

        PutModule("Please tell me what email address you want notifications to be sent to with 'email <address>'");

        VCString tokens;
        int token_count = sArgs.Split(" ", tokens, false);

        if (token_count < 1)
        {
            return true;
        }

        CString action = tokens[0].AsLower();

        if (action == "debug"){
            PutModule("DEBUG ON");
            DebugMode = true;
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

                PutModule("View the documentation at...");

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

            CString message = "Testing";
            MessagesList.push_front(message);
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

        if (SendNotification(Nick, sMessage) || location == "PRIVATE"){

            CString message = "<" + location + ":" + Nick.GetNick() + "> " + sMessage + "\n\n";
            MessagesList.push_front(message);

            DebugPrint("Added message...");
            DebugPrint(message);

            if (MessagesList.size() > MaxNotifications){
                MessagesList.pop_back();
            }

        }
    }

    bool SendNotification(CNick& Nick, CString& sMessage){

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

        if (!DebugMode && ConnectedUser->IsUserAttached()){
            return;
        }

        list<CString>::iterator it;

        CString message;

        if (MessagesList.size() <= 0){
            return;
        }

        for ( it=MessagesList.begin() ; it != MessagesList.end(); it++ ){
            message = message + *it;
        }

        Send(message);

        MessagesList.erase(MessagesList.begin(),MessagesList.end());

    }

    void Send(CString& sMessage){

        CString command;
        command = "mail -s '" + NotificationSubject + "' " + NotificationEmail;

        if (DebugMode){
            DebugPrint(command);
        }

        DebugPrint("Sending");

        FILE *email= popen(command.c_str(), "w");

        if (!email){
            PutModule("Problems with mail command.");
            return;
        }

        fprintf(email, "%s", (char*) sMessage.c_str());
        pclose(email);

        DebugPrint(sMessage);
        DebugPrint("Sent");

    }

};

void CMailerTimer::RunJob(){

    CMailer *p = (CMailer *)m_pModule;

    p->BatchSend();

}

MODULEDEFS(CMailer, "To be used to send mentions as an email")
