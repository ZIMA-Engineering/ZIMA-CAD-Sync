#include "Item.h"

Item::Item()
	: fd(0)
{
}

Item::~Item()
{
	if(fd)
		delete fd;

	qDeleteAll(children);
}
