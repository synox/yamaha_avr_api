#include <iostream>
#include <string>
#include <vector>

using namespace std;

/**
 * @brief The YamahaControl class provides an API to the yamaha RX-V_71 (and A_10) series AVR.
 */
class YamahaControl {
public:
    YamahaControl(string hostname) : hostname(hostname) {}
    void runCommand(vector<string> options, string value);
private:
    string hostname;
    const string OPEN_TAG  = "<";
    const string CLOSE_TAG  = ">";
    const string SLASH  = "/";
};

void YamahaControl::runCommand(vector<string> options, string value) {
    // work from inside to outside: first value, then last option, second last option, and so on.

    string optionString;

    // value
    optionString.append(value);

    for (vector<string>::reverse_iterator i = options.rbegin(); i != options.rend(); i++) {
        optionString.insert(0, OPEN_TAG + *i + CLOSE_TAG);
        optionString.append(OPEN_TAG +SLASH + *i + CLOSE_TAG);
    }


    optionString.insert(0, R"(<YAMAHA_AV cmd=\"PUT\">)");
    optionString.append("</YAMAHA_AV>");

    string url = string("http://").append(this->hostname).append("/YamahaRemoteControl/ctrl");

    string command;
    command.append(R"(/usr/bin/curl --data ")" );
    command.append(optionString);
    command.append(R"(" )");
    command.append(url);

    system(command.c_str());
    cout << endl;
}



int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <hostname> <option>* <value>" << endl;
        cout << "       sets the value on the yamaha device" << endl;
        cout << "       example: Main_Zone Volume Lvl \"<Val>Down 2 dB</Val><Exp></Exp><Unit></Unit>\" " << endl;
        cout << "                Main_Zone 2Volume Mute On/Off " << endl;

        return -1;
    }

    char* hostname = argv[1];
    YamahaControl l(hostname);

    // copy options
    vector<string> options;
    for(int i=2; i < argc-1; i++) {
        options.push_back(argv[i]);
    }

    // value
    string value = std::string(argv[argc-1]);

    // run
    l.runCommand(options, value);

    return 0;
}
