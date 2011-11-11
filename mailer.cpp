#include <main.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <Modules.h>
#include <IRCSock.h>
#include <User.h>
#include <Nick.h>
#include <Chan.h>

class CMailer : public CModule {

protected:
    CUser *user;

public:

    MODCONSTRUCTOR(CMailer) {
        user = GetUser();
    }

    virtual ~CMailer() {}

    virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {

        CString user_nick = user->GetNick().AsLower();

        if (user->IsUserAttached()){
            return CONTINUE;
        }

        size_t found = sMessage.AsLower().find(user_nick);

        if (found!=string::npos){

            PutModule("Sending");

            FILE *email= popen("mail -s 'IRC Notification' dougal85@gmail.com", "w");

            if (!email){
                PutModule("Problems with pipe");
                return CONTINUE;
            }

            CString message = "<" + Nick.GetNick() + "> " + sMessage + "\n\n";
            PutModule(message);
            fprintf(email, "%s", (char*) message.c_str());
            pclose(email);

            PutModule("Sent");

        }

        return CONTINUE;
    }

    bool send_notification(){
        return false;

    }

};

MODULEDEFS(CMailer, "To be used to send mentions as an email")
