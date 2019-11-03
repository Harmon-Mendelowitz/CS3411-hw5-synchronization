#include "lock.h"
#include "types.h"
#include "user.h"

int
lock_create(lock_type_t type)
{
    
    return slock_create(type);
}
int
lock_take(int lockid)
{

    return slock_take(lockid);
}
int
lock_release(int lockid)
{

    return slock_release(lockid);
}
void
lock_delete(int lockid)
{
    slock_delete(lockid);
}