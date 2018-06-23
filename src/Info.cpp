#include "Info.h"

namespace Cog
{

Symbol::Symbol()
{
	scope = -1;
	value = NULL;
}

Symbol::~Symbol()
{
}

Typename::Typename()
{
	type = NULL;
}

Typename::~Typename()
{
}

Info::Info()
{
	type = NULL;
	value = NULL;
	symbol = -1;
}

Info::~Info()
{
}

}
