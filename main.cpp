#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <ncurses.h>
#include "pugixml.hpp"
#include "status.hpp"


using namespace std;

#define BUFFER_SIZE (256 * 1024) /* 256kB */
typedef struct point point_type;
struct http_response {
    char data [BUFFER_SIZE];
    int pos = 0;
};



class Http {
private:
    CURL* curl;
    struct curl_slist* slist;
    string response;

    static size_t read_response( void *ptr, size_t size, size_t nmemb, void *stream) {

        struct http_response* result = (struct http_response*)stream;

        /* Will we overflow on this write? */
        if(result->pos + size * nmemb >= BUFFER_SIZE - 1) {
            fprintf(stderr, "curl error: too small buffer\n");
            return 0;
        }

        /* Copy curl's stream buffer into our own buffer */
        memcpy(result->data + result->pos, ptr, size * nmemb);

        /* Advance the position */
        result->pos += size * nmemb;

        return size * nmemb;
    }


public:
    Http() {
        curl = curl_easy_init();
        slist = curl_slist_append(NULL, "Content-Type: text/xml; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT , 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_response);
    }
    ~Http() {
        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
    }
    bool request(string& url, string xmlString) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST , 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xmlString.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE  , xmlString.size());

        // Create the write buffer
        struct http_response write_result;

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

        CURLcode res = curl_easy_perform(curl);
        /* null terminate the string */
        write_result.data[write_result.pos] = '\0';

        if(res != CURLE_OK) {
            response = string("curl_easy_perform() failed: ") +  curl_easy_strerror(res);
            return false;
        }


        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200 && res != CURLE_ABORTED_BY_CALLBACK)  {
            //Succeeded
            response = string(write_result.data);
        }  else        {
            //Failed
            string err = string("curl_easy_perform() failed: http response code: ") +  std::to_string(http_code) + string(write_result.data);
            throw err;
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
    int currentNetRadioStation=1;
public:
    YamahaControl(char* hostname) {
        url = string("http://").append(hostname).append("/YamahaRemoteControl/ctrl").c_str();
    }

    Status* getStatus() {
        try {
            string basicResponse = run("<Main_Zone><Basic_Status>GetParam</Basic_Status></Main_Zone>", "GET");
            string netResponse = run("<NET_RADIO><Play_Info>GetParam</Play_Info></NET_RADIO>", "GET");



        //        string basicResponse = R"(<YAMAHA_AV rsp="GET" RC="0"><Main_Zone><Basic_Status><Power_Control><Power>On</Power><Sleep>Off</Sleep></Power_Control><Volume><Lvl><Val>-480</Val><Exp>1</Exp><Unit>dB</Unit></Lvl><Mute>Off</Mute></Volume><Input><Input_Sel>NET RADIO</Input_Sel></Input></Basic_Status></Main_Zone></YAMAHA_AV>)";

            pugi::xml_document doc;
            pugi::xml_parse_result parseResult =doc.load_buffer(basicResponse.c_str(), basicResponse.length());
            if(parseResult)  {
                status.power = doc.select_single_node("//Main_Zone/Basic_Status/Power_Control/Power").node().child_value();
                const char* volumeString= doc.select_single_node("//Main_Zone/Basic_Status/Volume/Lvl/Val").node().child_value();
                if(strlen(volumeString)>0) {
                    status.volume = std::stod(volumeString) / 10;
                } else {
                    printf("parse volume error: %s", basicResponse.c_str());
                }

                status.mute = doc.select_single_node("//Main_Zone/Basic_Status/Volume/Mute").node().child_value();
                status.source = doc.select_single_node("//Main_Zone/Basic_Status/Input/Input_Sel").node().child_value();

                // TODO: parse xml
                status.station = doc.select_single_node("//NET_RADIO/Play_Info/Meta_Info/Station").node().child_value();
            } else {
//                cerr << "status xpath error: " << parseResult.description() << endl;
            }

            // <YAMAHA_AV rsp="GET" RC="0"><NET_RADIO><Play_Info><Feature_Availability>Ready</Feature_Availability><Playback_Info>Play</Playback_Info><Meta_Info><Station>SRF 3</Station><Album></Album><Song>Jetzt: World Music Special - Der Okzitanier-Blues</Song></Meta_Info><Album_ART><URL>/YamahaRemoteControl/AlbumART/AlbumART.ymf</URL><ID>25</ID><Format>YMF</Format></Album_ART></Play_Info></NET_RADIO></YAMAHA_AV>
            pugi::xml_parse_result parseResultNet =doc.load_buffer(netResponse.c_str(), basicResponse.length());
            if(parseResultNet)  {
                pugi::xpath_node node = doc.select_single_node("//NET_RADIO/Play_Info/Meta_Info/Station");
                status.station = node.node().child_value();
            } else {
//                cout << "NET_RADIO xpath error: " << parseResultNet.description() << endl;
            }

        } catch (pugi::xpath_exception e) {
            cerr << "can not parse status" << e.what() << endl;
            return NULL;
        } catch (char* e) {
            cerr << "can not parse status" << e << endl;
            return NULL;
        }

        return &status;
    }

    string selectInput(string inputName, int suffix) {
        inputName.append(std::to_string(suffix));
        return selectInput(inputName);
    }

    string selectInput(string inputName) {
        return run({"Main_Zone", "Input", "Input_Sel"} ,inputName, "PUT");
    }

    string shiftNetStation(int amount) {
        currentNetRadioStation += amount;
        return run( {"NET_RADIO","List_Control","Direct_Sel"} ,string("Line_")+std::to_string(currentNetRadioStation), "PUT");
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
    mvaddstr(0, 0, "commands: ");
    addch('O'  | A_UNDERLINE); printw("n/off, ");
    addch('^'  | A_UNDERLINE); addch('v'  | A_UNDERLINE); printw("up/down, ");
    addch('m'  | A_UNDERLINE); printw("ute, ");
    addch('n'  | A_UNDERLINE); printw("et radio, ");
    addch('<'  | A_UNDERLINE); addch('>'  | A_UNDERLINE); printw("next/prev station");

    attron(A_BOLD);
    mvaddstr(2, 33, "Yamaha Control\n");
    attroff(A_BOLD);
    refresh();

    timeout(1000); //ms
    char ch;
    while((ch = getch()) != 0)
    {

        string r;
        string name;
        if(ch != ERR) {
            switch(ch) {
            case 3: name="up"; r=control.run( {"Main_Zone", "Volume", "Lvl"} ,"<Val>Up 2 dB</Val><Exp></Exp><Unit></Unit>", "PUT");  break;
            case 2: name="down";r=control.run( {"Main_Zone", "Volume", "Lvl"} ,"<Val>Down 2 dB</Val><Exp></Exp><Unit></Unit>", "PUT");  break;
            case 4: name="next"; control.shiftNetStation(-1);break;
            case 5: name="next"; control.shiftNetStation(1); break;
            case 'm': name="mute_on_off";r=control.run( {"Main_Zone", "Volume", "Mute"},"On/Off", "PUT");  break;
            case '0': name="net_radio_0";;  break;
            case 'o': name="on_off";r=control.run( "<Main_Zone><Power_Control><Power>On/Standby</Power></Power_Control></Main_Zone>", "PUT");  break;
            case 'n': name="net_radio"; r=control.selectInput("NET RADIO");
                r = control.shiftNetStation(0); // load init station
                break;
            case '1': name="hdmi1";r=control.selectInput("HDMI",1);  break;
            case '2': name="hdmi2";r=control.selectInput("HDMI",2);  break;
            case '3': name="hdmi3";r=control.selectInput("HDMI",3);  break;
            case '4': name="hdmi4";r=control.selectInput("HDMI",4);  break;
            case '5': name="hdmi5";r=control.selectInput("HDMI",5);  break;
            case ' ': name="refresh";r=""; break;
            default:
                mvprintw(12,33," - key: %d - %s\n",ch, keyname( ch ));
                continue;
            }

            if(!name.empty()) {
                mvprintw(15,33," - %s -\n",name.c_str());
            }
        } else {
            // remove old text
            mvprintw(12,33,"\n");
        }


        Status* status = control.getStatus();
        if(status->mute.compare("On")==0){
            mvprintw(6,25,"%10s:  %s\n", "Power","Mute");
        } else {
            mvprintw(6,25,"%10s:  %s\n", "Power",status->power.c_str());
        }
        mvprintw(7,25,"%10s:  %2.1f\n", "Volume", status->volume);
        //        mvprintw(7,25,"%10s:  %s\n", "Mute", status->mute.c_str());
        mvprintw(8,25,"%10s:  %s\n", "Input", status->source.c_str());
        mvprintw(9,25,"%10s:  %s\n", "Station", status->station.c_str());

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
