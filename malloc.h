/*
 * malloc.h
 *
 * Created: 2013-01-27 13:18:24
 *  Author: tmf
 */ 


#ifndef MALLOC_H_
#define MALLOC_H_

#include <util/atomic.h>
#include <stdlib.h>

static inline void *malloc_re(size_t size)    //Funkcja omijaj¹ca problem braku wspó³bie¿noœci funkcji malloc
{
	void *ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ret=malloc(size);
	}
	return ret;
}


#endif /* MALLOC_H_ */