#include "cgi.h"

#include "cfg.h"
#include "photohosting.h"


Cgi::Cgi(const Config &cfg, Photohosting *_photohosting)
    : photohosting(_photohosting)
{
}


Cgi::~Cgi()
{
}


bool Cgi::Init()
{
    return true;
}


void Cgi::Routine()
{

}

