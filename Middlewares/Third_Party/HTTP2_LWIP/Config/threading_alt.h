/* Mutex protection for multi threaded mbedtls */
#include "cmsis_os.h"

#ifndef __THREADING_ALT_H
#define __THREADING_ALT_H
/* Typedef void *__iar_Rmtx;                   // Lock info object */
typedef SemaphoreHandle_t mbedtls_threading_mutex_t;
void mutex_init (mbedtls_threading_mutex_t  * mutex );
void mutex_free (mbedtls_threading_mutex_t  * mutex );
int  mutex_lock (mbedtls_threading_mutex_t * mutex);
int  mutex_unlock (mbedtls_threading_mutex_t * mutex);


void mbedtls_threading_set_alt( void (*mutex_init)( mbedtls_threading_mutex_t * ),
                                void (*mutex_free)( mbedtls_threading_mutex_t * ),
                                int (*mutex_lock)( mbedtls_threading_mutex_t * ),
                                int (*mutex_unlock)( mbedtls_threading_mutex_t * ) );

#endif
