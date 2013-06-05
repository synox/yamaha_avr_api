#ifndef ACTIONRUNNER_H
#define ACTIONRUNNER_H

#include <string>
#include "status.hpp"

using namespace std;

class ActionRunner
{
public:
    virtual string runAction(string keyword)=0;
    virtual Status getStatus()=0;
};

#endif // ACTIONRUNNER_H
