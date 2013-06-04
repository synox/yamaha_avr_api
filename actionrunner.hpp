#ifndef ACTIONRUNNER_H
#define ACTIONRUNNER_H

#include <string>

using namespace std;

class ActionRunner
{
public:
    virtual string runAction(string keyword)=0;
};

#endif // ACTIONRUNNER_H
