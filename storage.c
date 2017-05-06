#include "storage.h"

struct storage_device *storage[MAX_STORAGE_DEVICE];

int register_storage_device(struct storage_device *sd,unsigned int num)
{
	if(num>MAX_STORAGE_DEVICE)
		return -1;

	if(storage[num]){
		return -1;
	}else{
		storage[num]=sd;
	}
	return 0;
};
