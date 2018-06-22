#include "CogInfo.h"

namespace Cog
{

bool operator<(Typename t1, Typename t2)
{
	return t1.id < t2.id;
}

bool operator>(Typename t1, Typename t2)
{
	return t1.id > t2.id;
}

bool operator<=(Typename t1, Typename t2)
{
	return t1.id <= t2.id;
}

bool operator>=(Typename t1, Typename t2)
{
	return t1.id >= t2.id;
}

bool operator==(Typename t1, Typename t2)
{
	return t1.id == t2.id;
}

bool operator!=(Typename t1, Typename t2)
{
	return t1.id != t2.id;
}

}
