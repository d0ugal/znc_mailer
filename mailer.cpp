#include <main.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <Modules.h>
#include <Nick.h>
#include <IRCNetwork.h>

class Mailer : public CModule {

public:

    MODCONSTRUCTOR(Mailer) {}
    virtual ~Mailer() {}

    virtual void OnModCommand(const CString& sCommand) {
        if (sCommand.Token(0).CaseCmp("HELP") == 0) {
            PutModule("I don't know how to help");
        } else {
            PutModule("Unknown command, try HELP");
        }
    }

    virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {

        if (m_pNetwork->IsUserAttached()){
            return CONTINUE;
        }

        size_t found;

        found = sMessage.find("d0ugal");

        if (found!=string::npos){

            FILE *email= popen("mail -s 'IRC Notification' dougal85@gmail.com", "wb");
            string message = "<" + Nick.GetNick() + ">\n\n" + sMessage;
            PutModule(message.c_str());
            fprintf(email, "%s", (char*) message.c_str());
            pclose(email);
        }

        return CONTINUE;
    }

    virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage) {

        if (m_pNetwork->IsUserAttached()){
            return CONTINUE;
        }

        FILE *email= popen("mail -s 'IRC Notification' dougal85@gmail.com", "wb");
        string message = "<" + Nick.GetNick() + ">\n\n" + sMessage;
        PutModule(message.c_str());
        fprintf(email, "%s", (char*) message.c_str());
        pclose(email);
        return CONTINUE;
    }

};

MODULEDEFS(Mailer, "To be used to send mentions as an email")
