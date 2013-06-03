#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;


class Cmd {
private:
    vector<string> options;
    string value;
    const string OPEN_TAG  = "<";
    const string CLOSE_TAG  = ">";
    const string SLASH  = "/";
public:
    Cmd(vector<string> options, string value): options(options), value(value) {}

    string toString() {
        // work from inside to outside: first value, then last option, second last option, and so on.

        string optionString;

        // value
        optionString.append(value);

        // reverse order!
        for_each(options.rbegin(), options.rend(), [&](string option){
            optionString.insert(0, OPEN_TAG + option + CLOSE_TAG);
            optionString.append(OPEN_TAG +SLASH + option + CLOSE_TAG);
        });

        optionString.insert(0, R"(<YAMAHA_AV cmd=\"PUT\">)");
        optionString.append("</YAMAHA_AV>");

        return optionString;
    }
};

/**
 * @brief The YamahaControl class provides an API to the yamaha RX-V_71 (and A_10) series AVR.
 */
class YamahaControl {
public:
    YamahaControl(string hostname) {
        url = string("http://").append(hostname).append("/YamahaRemoteControl/ctrl");
    }
    void runCommand(Cmd& cmd);
    bool runAction(string keyword) {
        auto iterator= actions.find(keyword);
        if(iterator!=actions.end()) {
            pair<string,Cmd> pair = *iterator;
            Cmd cmd = pair.second;
            cout << "running command "<<keyword<<endl;
            runCommand(cmd);
            return true;
        } else {
            return false;
        }
    }

    void addAction(string keyword, vector<string> options, string value) {
        Cmd cmd(options,value);
        map<string, Cmd>::value_type pair(keyword, cmd);
        actions.insert(pair);
    }

    void loadActions() {
        addAction("down", {"Main_Zone", "Volume", "Lvl"} ,"<Val>Down 2 dB</Val><Exp></Exp><Unit></Unit>");
        addAction("up",   {"Main_Zone", "Volume", "Lvl"} ,"<Val>Up 2 dB</Val><Exp></Exp><Unit></Unit>");
    }

private:
    string url;
    map<string,Cmd> actions;
};

void YamahaControl::runCommand(Cmd& cmd) {

    string command = R"(/usr/bin/curl --data ")";
    command.append(cmd.toString());
    command.append(R"(" )");
    command.append(url);

    system(command.c_str());
    cout << endl;
}




int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <hostname> <action>" << endl;
        cout << "       sets the value on the yamaha device" << endl;
        cout << "       example: 192.168.1.45 up" << endl;
        return -1;
    }


    char* hostname = argv[1];
    YamahaControl control(hostname);
    control.loadActions();


    // name
    string action = std::string(argv[2]);
    control.runAction(action);

    return 0;
}
