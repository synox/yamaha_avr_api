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

    printw("Interactive Mode\n");
    refresh();

    noecho(); // dont' show typed letters

    char ch;
    while((ch = getch()) != 0)
    {
        string action;
        if(ch==3) action ="up";
        if(ch==4) action ="left";
        if(ch==5) action ="right";
        if(ch==2) action ="down";
        if(ch=='.') action ="dot";

//            printw("\nThe pressed key is ");
//            attron(A_BOLD);
//            printw("%d", ch);
//            attroff(A_BOLD);

        if(!action.empty() ) {
            clear();
            printw("running command %s\n",action.c_str());
            control.runAction(action);
            clear();
            printw("command %s done.\n",action.c_str());
        }
    }

    /*  Clean up after ourselves  */

    delwin(mainwin);
    endwin();
    refresh();
}
