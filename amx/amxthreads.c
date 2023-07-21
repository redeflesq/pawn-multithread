/*  Threads module for the Pawn AMX
 *
 *  Copyright (c) CompuPhase, 1997-2020, redeflesq 2023
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: amxthreads.c ... 2023-07-21 19:47:15Z ... $
 */
#if defined _UNICODE || defined __UNICODE__ || defined UNICODE
# if !defined UNICODE   /* for Windows */
#   define UNICODE
# endif
# if !defined _UNICODE  /* for C library */
#   define _UNICODE
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "osdefs.h"
#if defined __ECOS__
  /* eCos puts include files in cyg/package_name */
  #include <cyg/pawn/amx.h>
  #define  stricmp(a,b) strcasecmp(a,b)
#else
  #include "amx.h"
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32 || defined _Windows
  #include <windows.h>
#endif

/* A few compilers do not provide the ANSI C standard "time" functions */
#if !defined SN_TARGET_PS2 && !defined _WIN32_WCE && !defined __ICC430__
  #include <time.h>
#endif

#define THREADS_BLOCK 8

typedef struct amx_thread
{
    unsigned int index;

    struct {
        AMX* amx;
        cell* retval;
        int index;
    } vm;

    struct {
        HANDLE handle;
        DWORD id;
    } winapi;

} amx_thread, *amx_pthread;

static amx_pthread* threads;
static int threads_index = 0;
static int threads_size = 0;

amx_pthread impl_find_thread(unsigned int index)
{
    for (unsigned int i = 0; i < threads_index; i++) {
        if (threads[i] == NULL)
            continue;

        if (threads[i]->index == index) {
            return *(threads + i);
        }
    }

    return NULL;
}

DWORD WINAPI impl_amx_exec(LPVOID lParam)
{
    int thread_index = (int)lParam;
    amx_pthread thread = impl_find_thread(thread_index);

    if (!thread)
        return 0;

    AMX* amx = thread->vm.amx;
    cell* retval = thread->vm.retval;
    int index = thread->vm.index;

    amx_Push(amx, (cell)lParam);
    int err = amx_Exec(amx, retval, index);
    if (err != AMX_ERR_NONE) {
        amx_RaiseError(amx, err);
    }

    return 0;
}

static cell AMX_NATIVE_CALL threads_create_thread(AMX* amx, const cell* params)
{
    int amxerr = AMX_ERR_NONE;
    int idx = 0;

    cell* addr = amx_Address(amx, params[1]);

    if (!amx_VerifyAddress(amx, addr)) {
        amx_RaiseError(amx, AMX_ERR_MEMORY);
        return 0;
    }

    int len = 0;
    if ((amxerr = amx_StrLen(addr, &len)) != AMX_ERR_NONE) {
        amx_RaiseError(amx, amxerr);
        return 0;
    }

    unsigned char p_func[64] = { 0 };

    if ((amxerr = amx_GetString(p_func, addr, 0, len + 1)) != AMX_ERR_NONE) {
        amx_RaiseError(amx, amxerr);
        return 0;
    }

    if ((amxerr = amx_FindPublic(amx, p_func, &idx)) != AMX_ERR_NONE) {
        amx_RaiseError(amx, amxerr);
        return 0;
    }

    DWORD thread_id = 0;

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)impl_amx_exec, threads_index, CREATE_SUSPENDED, &thread_id);

    if (!hThread) {
        return 0;
    }

    if (threads_index >= threads_size - 1) {
        threads_size += THREADS_BLOCK;
        void* new_threads = realloc(threads, threads_size * sizeof(amx_pthread));
        if (!new_threads) {
            amx_RaiseError(amx, AMX_ERR_MEMORY);
            return 0;
        }
        threads = new_threads;
        for (int i = threads_index; i < threads_size; i++) {
            threads[i] = malloc(sizeof(amx_thread));
            if (!threads[i]) {
                amx_RaiseError(amx, AMX_ERR_MEMORY);
                return 0;
            }
        }
    }

    threads[threads_index]->index = threads_index;

    threads[threads_index]->vm.amx = amx;
    threads[threads_index]->vm.index = idx;
    threads[threads_index]->vm.retval = malloc(sizeof(cell));

    threads[threads_index]->winapi.handle = hThread;
    threads[threads_index]->winapi.id = thread_id;

    threads_index++;

    return threads_index;

}

static cell AMX_NATIVE_CALL threads_destroy_thread(AMX* amx, const cell* params)
{
    amx_pthread thread = impl_find_thread(params[1] - 1);

    if (!thread)
        return 0;

    CloseHandle(thread->winapi.handle);

    int index = thread->index;

    free(threads[index]);
    
    threads[index] = NULL;

    return threads[index] == NULL;
}

static cell AMX_NATIVE_CALL threads_resume_thread(AMX* amx, const cell* params)
{
    (void)amx;

    amx_pthread thread = impl_find_thread(params[1] - 1);

    if (!thread) 
        return 0;
    
    return ResumeThread(
        thread->winapi.handle
    );
}

static cell AMX_NATIVE_CALL threads_suspend_thread(AMX* amx, const cell* params)
{
    (void)amx;

    amx_pthread thread = impl_find_thread(params[1] - 1);

    if (!thread)
        return 0;

    return SuspendThread(
        thread->winapi.handle
    );
}

static cell AMX_NATIVE_CALL threads_wait_thread(AMX* amx, const cell* params)
{
    (void)amx;

    amx_pthread thread = impl_find_thread(params[1] - 1);

    if (!thread)
        return 0;

    return WaitForSingleObject(thread->winapi.handle, params[2]);
}


#if defined __cplusplus
  extern "C"
#endif
const AMX_NATIVE_INFO Threads_Natives[] = {
  { "create_thread",        threads_create_thread },
  { "destroy_thread",       threads_destroy_thread },
  { "resume_thread",        threads_resume_thread },
  { "suspend_thread",       threads_suspend_thread },
  { "wait_thread",          threads_wait_thread },

  { NULL, NULL }        /* terminator */
};

int AMXEXPORT AMXAPI amx_ThreadsInit(AMX *amx)
{
    threads_size = THREADS_BLOCK;

    threads = malloc(sizeof(amx_pthread) * threads_size);
    if (!threads)
        return AMX_ERR_MEMORY;

    for (int i = 0; i < threads_size; i++) {
        threads[i] = malloc(sizeof(amx_thread));

        if (!threads[i])
            return AMX_ERR_MEMORY;
    }
    
    return amx_Register(amx, Threads_Natives, -1);
}

int AMXEXPORT AMXAPI amx_ThreadsCleanup(AMX *amx)
{
    (void)amx;

    if (!threads)
        return AMX_ERR_NONE;

    for (int i = 0; i < threads_size; i++)
    {
        if (threads[i] == NULL)
            continue;

        CloseHandle(threads[i]->winapi.handle);

        free(threads[i]->vm.retval);
        free(threads[i]);
    }

    free(threads);
  
    return AMX_ERR_NONE;
}
