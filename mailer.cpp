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
    CUser *user;
    list<CString> messages_list;
    CString notification_subject;
    CString notification_email;
    unsigned int max_notifications;

public:

    MODCONSTRUCTOR(CMailer) {
        user = GetUser();
    }

    virtual ~CMailer() {}


    virtual bool OnLoad(const CString& sArgs, CString& sErrorMsg) {
        AddTimer(new CMailerTimer(this, 1200, 0, "Mail", "Send emails every 20 mins."));
        this->max_notifications = 50;
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

    virtual void OnIRCConnected() {
        PutModule("Connected");
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
                PutModule("Usage: email <email address>");
                return;
            }

            this->notification_email = tokens[1].AsLower();

            PutModule("email address set");

        } else if (action == "subject"){

            if (token_count < 2)
            {
                PutModule("Usage: subject <email subject>");
                return;
            }

            this->notification_subject = tokens[1].AsLower();

            PutModule("email subject set.");
        } else if (action == "help") {

                PutModule("View the documentation at https://github.com/d0ugal/znc_mailer/blob/master/README");

        } else {

            PutModule("Error: invalid command, try `help`");

        }

    }

    void Handle(CNick& Nick, const CString& location, CString& sMessage){

        if (SendNotification(Nick, sMessage) || location == "PRIVATE"){

            CString message = "<" + location + ":" + Nick.GetNick() + "> " + sMessage + "\n\n";
            messages_list.push_front(message);

            if (messages_list.size() > this->max_notifications){
                messages_list.pop_back();
            }

        }
    }

    bool SendNotification(CNick& Nick, CString& sMessage){

        if (user->IsUserAttached()){
            return false;
        }

        CString UserNick = user->GetNick().AsLower();

        size_t found = sMessage.AsLower().find(UserNick);

        if (found!=string::npos){
            return true;
        } else {
            return false;
        }

    }

    void BatchSend(){

        if (user->IsUserAttached()){
            return;
        }

        list<CString>::iterator it;

        CString message;

        if (messages_list.size() <= 0){
            return;
        }

        for ( it=messages_list.begin() ; it != messages_list.end(); it++ ){
            message = message + *it;
        }

        Send(message);

        messages_list.erase(messages_list.begin(),messages_list.end());

    }

    void Send(CString& sMessage){

        PutModule("Sending");

        FILE *email= popen(("mail -s '" + this->notification_email + "' " + this->notification_subject).c_str(), "w");

        if (!email){
            PutModule("Problems with pipe");
            return;
        }

        fprintf(email, "%s", (char*) sMessage.c_str());
        pclose(email);

        PutModule("Sent");

    }

};

void CMailerTimer::RunJob(){

    CMailer *p = (CMailer *)m_pModule;

    p->BatchSend();

}

MODULEDEFS(CMailer, "To be used to send mentions as an email")
