#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <ncurses.h>
#include "pugixml.hpp"
#include "status.hpp"


using namespace std;

class Http {
private:
    CURL* curl;
    struct curl_slist* slist;
    string response;

    /**
     * @brief readHttpResponse reads the http response and writes to the string
     */
    size_t readHttpResponse(void *ptr, size_t size, size_t count, string *stream) {
        stream->append((char*)ptr, 0, size*count);
        return size*count;
    }
public:
    Http() {
        curl = curl_easy_init();
        slist = curl_slist_append(NULL, "Content-Type: text/xml; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT , 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response); // used in readHttpResponse
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Http::readHttpResponse);

    }
    ~Http() {
        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
    }
    bool request(string& url, string& xmlString) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST , 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xmlString.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE  , xmlString.length());

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            response = string("curl_easy_perform() failed: ") +  curl_easy_strerror(res);
            return false;
        }
        return true;
    }
    string& getResponse() {return response;}


};


/**
 * @brief The YamahaControl class provides an API to the yamaha RX-V_71 (and A_10) series AVR.
 */
class YamahaControl {
private:
    string url;
    Http http;
    Status status;
public:
    YamahaControl(char* hostname) {
        url = string("http://").append(hostname).append("/YamahaRemoteControl/ctrl").c_str();
    }

    Status* getStatus() {
                string basicResponse = run("<Main_Zone><Basic_Status>GetParam</Basic_Status></Main_Zone>", "GET");
                string parseResultNet = run("<NET_RADIO><Play_Info>GetParam</Play_Info></NET_RADIO>", "GET");

//        string basicResponse = R"(<YAMAHA_AV rsp="GET" RC="0"><Main_Zone><Basic_Status><Power_Control><Power>On</Power><Sleep>Off</Sleep></Power_Control><Volume><Lvl><Val>-480</Val><Exp>1</Exp><Unit>dB</Unit></Lvl><Mute>Off</Mute></Volume><Input><Input_Sel>NET RADIO</Input_Sel></Input></Basic_Status></Main_Zone></YAMAHA_AV>)";

        try {
            pugi::xml_document doc;
            pugi::xml_parse_result parseResult =doc.load_buffer(basicResponse.c_str(), basicResponse.length());
            if(parseResult)  {
                status.power = doc.select_single_node("//Main_Zone/Basic_Status/Power_Control/Power").node().child_value();
                status.volume = doc.select_single_node("//Main_Zone/Basic_Status/Volume/Lvl/Val").node().child_value();
                status.mute = doc.select_single_node("//Main_Zone/Basic_Status/Volume/Mute").node().child_value();
                status.source = doc.select_single_node("//Main_Zone/Basic_Status/Input/Input_Sel").node().child_value();

                // TODO: parse xml
                status.station = doc.select_single_node("//NET_RADIO/Play_Info/Meta_Info/Station").node().child_value();
            } else {
                cerr << "can not load xml buffer for xpath" << parseResult.description() << endl;
            }

            pugi::xml_parse_result parseResultNet =doc.load_buffer(basicResponse.c_str(), basicResponse.length());
            if(parseResultNet)  {
                status.station = doc.select_single_node("//NET_RADIO/Play_Info/Meta_Info/Station").node().child_value();
            } else {
                cerr << "can not load xml buffer NET_RADIO for xpath" << parseResultNet.description() << endl;
            }




        } catch (pugi::xpath_exception e) {
            cerr << "can not parse status" << e.what() << endl;
            return NULL;
        }

        //        status.power = "On";
        //        status.source = "NET";
        //        status.volume = "-50.0 dB";
        //        status.station = "ABC";
        return &status;
    }

    string selectInput(string inputName, int suffix) {
        inputName.append(std::to_string(suffix));
        return selectInput(inputName);
    }

    string selectInput(string inputName) {
        return run({"Main_Zone", "Input", "Input_Sel"} ,inputName, "PUT");
    }

    string run(vector<string> options, string value, const char* method) {
        string optionString;

        // value
        optionString.append(value);

        // reverse order!
        for_each(options.rbegin(), options.rend(), [&](string option){
            optionString.insert(0, "<" + option + ">");
            optionString.append("</" + option + ">");
        });

        return run(optionString, method);
    }

    string run(string optionString, const char* method) {
        string prefix = R"(<YAMAHA_AV cmd=")";
        prefix.append(method);
        prefix.append(R"(">)");

        optionString.insert(0, prefix);
        optionString.append("</YAMAHA_AV>");
        http.request(url, optionString);
        return http.getResponse();
    }
};


void runInteractiveMode(YamahaControl& control)
{
    WINDOW * mainwin;
    if ( (mainwin = initscr()) == NULL ) {
        fprintf(stderr, "Error initialising ncurses.\n");
        exit(EXIT_FAILURE);
    }
    noecho(); // dont' show typed letters
    keypad (mainwin, TRUE);
    mvaddstr(0, 0, "commands: o=on/off, up/down/m=volume, n=net_radio, 1-5=hdmi, left/right=station");
    attron(A_BOLD);
    mvaddstr(2, 33, "Yamaha Control\n");
    attroff(A_BOLD);
    refresh();

    char ch;
    while((ch = getch()) != 0)
    {
        string r;
        string name;
        switch(ch) {
        case 3: name="up"; r=control.run( {"Main_Zone", "Volume", "Lvl"} ,"<Val>Up 2 dB</Val><Exp></Exp><Unit></Unit>", "PUT");  break;
        case 2: name="down";r=control.run( {"Main_Zone", "Volume", "Lvl"} ,"<Val>Down 2 dB</Val><Exp></Exp><Unit></Unit>", "PUT");  break;

        case 'm': name="mute_on_off";r=control.run( {"Main_Zone", "Volume", "Mute"},"On/Off", "PUT");  break;
        case '0': name="net_radio_0";;  break;
        case 'o': name="on_off";r=control.run( "<Main_Zone><Power_Control><Power>On/Standby</Power></Power_Control></Main_Zone>", "PUT");  break;
        case 'n': name="net_radio";
            r=control.selectInput("NET");
            r=control.run( {"NET_RADIO","List_Control","Direct_Sel"} ,"Line_1", "PUT");
            break;
        case '1': name="hdmi1";r=control.selectInput("HDMI",1);  break;
        case '2': name="hdmi2";r=control.selectInput("HDMI",2);  break;
        case '3': name="hdmi3";r=control.selectInput("HDMI",3);  break;
        case '4': name="hdmi4";r=control.selectInput("HDMI",4);  break;
        case '5': name="hdmi5";r=control.selectInput("HDMI",5);  break;
        case ' ': name="refresh";r=""; break;
        }

        if(!name.empty()) {
            mvprintw(12,33," - %s - ",name.c_str());
        }

        if(!r.empty()) {
            mvprintw(14,10,"%s\n", r.c_str());
        } else {
            Status* status = control.getStatus();
            mvprintw(6,25,"%10s:  %s\n", "Power",status->power.c_str());
            mvprintw(7,25,"%10s:  %s\n", "Volume", status->volume.c_str());
            mvprintw(7,25,"%10s:  %s\n", "Mute", status->mute.c_str());
            mvprintw(8,25,"%10s:  %s\n", "Input", status->source.c_str());
            mvprintw(9,25,"%10s:  %s\n", "Station", status->station.c_str());
        }

        move(0, 0);
        refresh();
    }

    /*  Clean up after ourselves  */

    delwin(mainwin);
    endwin();
    refresh();
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <hostname>" << endl;
        cout << "       sets the value on the yamaha device" << endl;
        cout << "       example: " << argv[0] <<" 192.168.1.45" << endl;
        return -1;
    }

    char* hostname = argv[1];

    YamahaControl control(hostname);
    runInteractiveMode(control);

    return 0;
}
