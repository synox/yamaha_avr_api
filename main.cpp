#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include "interactive_mode.hpp"
#include "actionrunner.hpp"


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

        optionString.insert(0, R"(<YAMAHA_AV cmd="PUT">)");
        optionString.append("</YAMAHA_AV>");

        return optionString;
    }
};

/**
 * @brief The YamahaControl class provides an API to the yamaha RX-V_71 (and A_10) series AVR.
 */
class YamahaControl: public ActionRunner {
public:
    YamahaControl(string hostname) {
        url = string("http://").append(hostname).append("/YamahaRemoteControl/ctrl").c_str();

    }

    static size_t readHttpResponse(void *ptr, size_t size, size_t count, string *stream);

    string runCommand(Cmd& cmd);
    string runAction(string keyword) {
        auto iterator= actions.find(keyword);
        if(iterator!=actions.end()) {
            pair<string,Cmd> pair = *iterator;
            Cmd cmd = pair.second;
            cout << "running command "<<keyword<<endl;
            return runCommand(cmd);
        } else {
            return string();
        }
    }

    void runKey(unsigned char c) {
        printf("%d\n", c);
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


/**
 * @brief YamahaControl::readHttpResponse reads the http response and writes to the string
 */
size_t YamahaControl::readHttpResponse(void *ptr, size_t size, size_t count, string *stream) {
    stream->append((char*)ptr, 0, size*count);
    return size*count;
}


string YamahaControl::runCommand(Cmd& cmd) {
    string response;

    CURL* curl = curl_easy_init();
    if(curl) {
        string xmlString = cmd.toString();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST , 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xmlString.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE  , xmlString.length());

        struct curl_slist *slist = curl_slist_append(NULL, "Content-Type: text/xml; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response); // used in readHttpResponse
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &readHttpResponse);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",  curl_easy_strerror(res));
        } else {
            cout << "response: "<< response << endl;
        }
        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
    }
    return response;
}


int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <hostname> <action/-i>" << endl;
        cout << "       sets the value on the yamaha device" << endl;
        cout << "       example: 192.168.1.45 up" << endl;
        cout << "       use action -i for interactive mode" << endl;
        return -1;
    }

    char* hostname = argv[1];
    string action = argv[2];

    YamahaControl control(hostname);
    control.loadActions();

    // interative
    if(action.compare("-i")==0) {
        runInteractiveMode(control);
    } else {
        // name
        string action = std::string(argv[2]);
        control.runAction(action);
    }

    return 0;
}
