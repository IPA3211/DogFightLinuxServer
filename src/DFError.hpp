#pragma once

#include <iostream>

class DFError
{
public:
    std::string Label;

    DFError() { Label = (char *)"Generic Error"; }
    DFError(char *message) { Label = message; }
    ~DFError() {}
    inline const char *GetMessage(void) { return Label.c_str(); }
};