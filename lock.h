#define MAX_NUM_LOCKS 16
#define LOCK_ADAPTIVE_SPIN 1024

typedef enum {
	LOCK_SPIN,
	LOCK_BLOCK,
	LOCK_ADAPTIVE,
} lock_type_t;

int  lock_create(lock_type_t type);
int  lock_take(int lockid);
int  lock_release(int lockid);
void lock_delete(int lockid);