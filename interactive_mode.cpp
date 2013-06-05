#include <ncurses.h>
#include <iostream>
#include "interactive_mode.hpp"
#include "actionrunner.hpp"


using namespace std;

void runInteractiveMode(ActionRunner& control)
{
    WINDOW * mainwin;
    if ( (mainwin = initscr()) == NULL ) {
        fprintf(stderr, "Error initialising ncurses.\n");
        exit(EXIT_FAILURE);
    }
    keypad (mainwin, TRUE);

    attron(A_BOLD);
    mvaddstr(2, 33, "Yamaha Control\n");
    attroff(A_BOLD);

    mvaddstr(0, 0, "commands: o=on/off, up/down/m=volume, n=net_radio, 1-5=hdmi, left/right=station");

    refresh();

    noecho(); // dont' show typed letters

    char ch;
    while((ch = getch()) != 0)
    {
        string action;
        // volume
        if(ch==3) action ="up";
        if(ch==2) action ="down";
        if(ch=='m') action ="mute_on_off";

        if(ch==4) action ="left";
        if(ch==5) action ="right";

        if(ch=='n') action ="net_radio";
        if(ch=='0') action ="net_radio_0";

        if(ch=='o') action ="on_off";
        if(ch=='1') action ="hdmi1";
        if(ch=='2') action ="hdmi2";
        if(ch=='3') action ="hdmi3";
        if(ch=='4') action ="hdmi4";
        if(ch=='5') action ="hdmi5";


        if(!action.empty() ) {
            mvprintw(13,33," - %s - ",action.c_str());
            refresh();
            string response = control.runAction(action);
            if(response.empty()) {
                addstr(" action not found or failed");
            } else {
                Status status = control.getStatus();
            }
            addstr("\n");
            move(0, 0);
            refresh();
        }
    }

    /*  Clean up after ourselves  */

    delwin(mainwin);
    endwin();
    refresh();
}
